#include "timesync.h"
#include "config.h"
#include "secrets.h"
#include <M5CoreS3.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_sntp.h>
#include <string.h>
#include <time.h>

enum class Phase { Wait, Connecting, WaitingNtp };
static Phase    phase  = Phase::Wait;
static uint32_t t0     = 0;
static uint32_t nextAt = 0;   // millis() a partir del cual se intenta
static char     statusText[64] = "Hora: solo RTC";
static uint8_t  failures = 0;   // fallos consecutivos del ciclo de reintentos actual
static bool     countdown = false;   // true: se muestra la cuenta regresiva hasta el próximo intento
static bool     gaveUp    = false;   // true: se agotaron los reintentos (próximo intento en el ciclo largo)

static void setStatus(const char* t) {
    if (strcmp(statusText, t) == 0) return;   // solo si cambia
    snprintf(statusText, sizeof(statusText), "%s", t);
    if (Serial) Serial.printf("[hora] %s\n", t);
}

// Texto de estado; si hay un intento programado, añade la cuenta regresiva (se actualiza sola).
const char* timeSyncStatus() {
    if (!countdown) return statusText;
    static char out[80];
    int32_t ms = (int32_t)(nextAt - millis());
    if (ms < 0) ms = 0;
    const unsigned s = (unsigned)((ms + 999) / 1000);   // redondeo hacia arriba
    char t[16];
    if (s < 100)       snprintf(t, sizeof(t), "%us", s);
    else if (s < 3600) snprintf(t, sizeof(t), "%um %02us", s / 60, s % 60);
    else               snprintf(t, sizeof(t), "%uh %02um", s / 3600, (s % 3600) / 60);
    snprintf(out, sizeof(out), "%s - %s %s", statusText, gaveUp ? "prox. en" : "reintento en", t);
    return out;
}

// ---- Conversión fecha <-> segundos, independiente de la zona horaria del sistema ----
// (la hora que guardamos es siempre la hora local de Bogotá)
static int64_t daysFromCivil(int y, int m, int d) {
    y -= m <= 2;
    const int64_t era = (y >= 0 ? y : y - 399) / 400;
    const int yoe = y - era * 400;
    const int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

static uint32_t toEpoch(int y, int mo, int d, int h, int mi, int s) {
    return (uint32_t)(daysFromCivil(y, mo, d) * 86400 + h * 3600 + mi * 60 + s);
}

static uint32_t buildEpoch() {
    static const char* months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    char mon[4] = {0};
    int day, year, h, m, s;
    sscanf(__DATE__, "%3s %d %d", mon, &day, &year);
    sscanf(__TIME__, "%d:%d:%d", &h, &m, &s);
    const char* p = strstr(months, mon);
    const int month = p ? (int)(p - months) / 3 + 1 : 1;
    return toEpoch(year, month, day, h, m, s);
}

static uint32_t rtcEpoch() {
    auto dt = CoreS3.Rtc.getDateTime();
    return toEpoch(dt.date.year, dt.date.month, dt.date.date, dt.time.hours, dt.time.minutes, dt.time.seconds);
}

static void setRtcFromEpoch(uint32_t e) {
    time_t t = (time_t)e;
    struct tm tm;
    gmtime_r(&t, &tm);   // gmtime no aplica zona horaria: devuelve los mismos campos que guardamos
    CoreS3.Rtc.setDateTime(&tm);
}

// ---- Última hora conocida (NVS) ----
static uint32_t lastSaveMs = 0;

static void saveLastKnownTime() {
    if (CoreS3.Rtc.getDateTime().date.year < RTC_MIN_VALID_YEAR) return;
    Preferences prefs;
    if (prefs.begin("clock", false)) {
        prefs.putULong("last", rtcEpoch());
        prefs.end();
    }
    lastSaveMs = millis();
}

static uint32_t loadLastKnownTime() {
    Preferences prefs;
    uint32_t e = 0;
    if (prefs.begin("clock", true)) {
        e = prefs.getULong("last", 0);
        prefs.end();
    }
    return e;
}

static void finish(bool ok, const char* msg) {
    WiFi.disconnect(true, false);
    WiFi.mode(WIFI_OFF);
    phase = Phase::Wait;

    if (ok) {
        failures = 0;
        countdown = false;
        nextAt = millis() + NTP_RESYNC_MS;
        saveLastKnownTime();
        setStatus(msg);
        return;
    }

    // Fallo: nunca se escribe en el RTC; se conserva la hora almacenada.
    countdown = true;                      // el estado muestra la cuenta regresiva al próximo intento
    if (failures < NTP_BACKOFF_COUNT) {
        nextAt = millis() + NTP_BACKOFF_MS[failures++];
        gaveUp = false;
        setStatus(msg);
    } else {
        failures = 0;                      // se agotaron los reintentos: espera al siguiente ciclo
        nextAt = millis() + NTP_RESYNC_MS;
        gaveUp = true;
        setStatus("Sin NTP: hora guardada");
    }
}

void timeSyncInit() {
    if (CoreS3.Rtc.getDateTime().date.year < RTC_MIN_VALID_YEAR) {
        // El RTC perdió la hora: usar la mayor entre la última guardada y la de compilación
        const uint32_t saved = loadLastKnownTime();
        const uint32_t built = buildEpoch();
        setRtcFromEpoch(saved > built ? saved : built);
        if (Serial) Serial.printf("[hora] RTC sin hora; restaurada desde %s\n", saved > built ? "NVS" : "la compilacion");
    }
    lastSaveMs = millis();
    nextAt = 0;   // primer intento NTP al arrancar
}

void timeSyncUpdate() {
    if (millis() - lastSaveMs >= TIME_SAVE_MS) saveLastKnownTime();
    if (WIFI_SSID[0] == '\0') { setStatus("Hora: solo RTC (sin Wi-Fi)"); return; }   // sin red configurada

    switch (phase) {
        case Phase::Wait:
            if ((int32_t)(millis() - nextAt) < 0) return;
            WiFi.persistent(false);   // no guardar SSID ni contraseña en la flash del equipo
            WiFi.mode(WIFI_STA);
            WiFi.setMinSecurity(WIFI_AUTH_WPA_PSK);   // por defecto exige WPA2; algunos routers anuncian WPA
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            t0 = millis();
            phase = Phase::Connecting;
            countdown = false;
            setStatus("Conectando Wi-Fi...");
            break;

        case Phase::Connecting:
            if (WiFi.status() == WL_CONNECTED) {
                // M5Unified copia la hora del RTC al reloj interno al arrancar, así que ese reloj
                // parece válido antes de tiempo: hay que esperar a que SNTP confirme la sincronización.
                sntp_set_sync_status(SNTP_SYNC_STATUS_RESET);
                configTzTime(TZ_BOGOTA, NTP_SERVER1, NTP_SERVER2);
                t0 = millis();
                phase = Phase::WaitingNtp;
                setStatus("Pidiendo hora (NTP)...");
            } else if (millis() - t0 > WIFI_TIMEOUT_MS) {
                const wl_status_t st = WiFi.status();
                char msg[48];
                if (st == WL_NO_SSID_AVAIL)       snprintf(msg, sizeof(msg), "Wi-Fi: red no encontrada");
                else if (st == WL_CONNECT_FAILED) snprintf(msg, sizeof(msg), "Wi-Fi: clave incorrecta");
                else                              snprintf(msg, sizeof(msg), "Wi-Fi: no conecta (cod %d)", (int)st);
                finish(false, msg);
            }
            break;

        case Phase::WaitingNtp: {
            struct tm tm;
            if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED && getLocalTime(&tm, 0) &&
                tm.tm_year + 1900 >= RTC_MIN_VALID_YEAR) {
                CoreS3.Rtc.setDateTime(&tm);   // el RTC guarda la hora local (Bogotá)
                finish(true, "Hora sincronizada (NTP)");
            } else if (millis() - t0 > WIFI_TIMEOUT_MS) {
                finish(false, "NTP: sin respuesta");
            }
            break;
        }
    }
}

void timeSyncAbort() {
    if (phase == Phase::Wait) return;
    WiFi.disconnect(true, false);
    WiFi.mode(WIFI_OFF);
    phase  = Phase::Wait;
    nextAt = millis();   // se retoma en cuanto se vuelva a llamar a timeSyncUpdate()
}
