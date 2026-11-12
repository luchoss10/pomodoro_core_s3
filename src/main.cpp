#include <M5CoreS3.h>
#include "config.h"
#include "orientation.h"
#include "pomodoro.h"
#include "settings.h"
#include "sound.h"
#include "timesync.h"
#include "ui.h"

static AppState    state = AppState::CLOCK;
static Settings    settings;
static Pomodoro    pomodoro;
static Orientation lastOrientation = Orientation::Base;
static bool        needsRedraw = true;
static uint32_t    lastTick = 0;
static int         lastMinute = -1;   // modo ahorro: minuto mostrado
static int         lastKey = -1;   // lo último que se dibujó (segundo del reloj o del Pomodoro)

// Solo se redibuja cuando cambia lo que se ve: cada dibujo cuesta ~50 ms de CPU.
static bool contentChanged() {
    static uint32_t lastCheck = 0;
    const uint32_t now = millis();
    switch (state) {
        case AppState::CLOCK: {                  // el segundero: una vez por segundo
            if (now - lastCheck < 100) return false;
            lastCheck = now;
            const int sec = CoreS3.Rtc.getDateTime().time.seconds;
            if (sec == lastKey) return false;
            lastKey = sec;
            return true;
        }
        case AppState::POMODORO: {               // la cuenta regresiva: una vez por segundo
            const int rem = (int)pomodoro.remainingSec();
            if (rem == lastKey) return false;
            lastKey = rem;
            return true;
        }
        default:                                 // calendario y ajustes: solo si hay cambios o de respaldo
            return now - lastTick >= IDLE_REDRAW_MS;
    }
}

static int minutesFor(Orientation o) {
    return o == Orientation::Left ? settings.shortMin : settings.longMin;
}

// Hay un Pomodoro pausado en el lado contrario al que se giró
static bool isBlocked(Orientation o) {
    return settings.imuEnabled
        && state != AppState::POMODORO && state != AppState::SETTINGS
        && o != Orientation::Base
        && pomodoro.state() == PomoState::Paused
        && pomodoro.side() != o;
}

static void onOrientationChange(Orientation o) {
    needsRedraw = true;
    if (!settings.imuEnabled || state == AppState::SETTINGS) return;

    // Al salir de su posición, la sesión en curso se pausa (conserva el progreso)
    if (pomodoro.state() == PomoState::Running && pomodoro.side() != o) pomodoro.pause();

    if (o == Orientation::Base) {
        if (pomodoro.state() == PomoState::Finished) pomodoro.stop();
        if (state == AppState::POMODORO) state = AppState::CLOCK;
        return;
    }

    switch (pomodoro.state()) {
        case PomoState::Idle:
        case PomoState::Finished:
            pomodoro.start(minutesFor(o), o);
            state = AppState::POMODORO;
            soundPlay(Sfx::Chime);
            break;
        case PomoState::Paused:
            if (pomodoro.side() == o) { pomodoro.resume(); state = AppState::POMODORO; }
            break;   // del otro lado: queda bloqueado (aviso)
        case PomoState::Running:
            break;
    }
}

static void applyBrightness() {
    CoreS3.Display.setBrightness(BRIGHTNESS_VALUES[settings.brightnessLevel]);
}

// Modo ahorro: solo la hora, brillo al mínimo, CPU a 80 MHz, sin sonido, IMU ni Wi-Fi
static void enterLowPower() {
    timeSyncAbort();   // si había una sincronización a medias, apaga el Wi-Fi
    state = AppState::LOW_POWER;
    lastMinute = -1;
    setCpuFrequencyMhz(LOW_POWER_CPU_MHZ);
    CoreS3.Display.setBrightness(LOW_POWER_BRIGHTNESS);
    needsRedraw = true;
}

static void exitLowPower() {
    setCpuFrequencyMhz(NORMAL_CPU_MHZ);
    applyBrightness();
    state = AppState::CLOCK;
    needsRedraw = true;
    lastTick = 0;
}

static void lowPowerLoop() {
    static uint32_t lastCheck = 0;
    if (millis() - lastCheck >= 1000) {
        lastCheck = millis();
        const int mi = CoreS3.Rtc.getDateTime().time.minutes;
        if (mi != lastMinute) { lastMinute = mi; needsRedraw = true; }   // una vez por minuto
    }
    if (needsRedraw) {
        needsRedraw = false;
        uiDraw(state, settings, pomodoro, Orientation::Base, false);
    }
    delay(100);
}

static void handleTouch() {
    auto t = CoreS3.Touch.getDetail();
    if (!t.wasPressed()) return;

    bool settingsChanged = false;   // ajuste modificado: se guarda en NVS al final

    switch (uiHit(t.x, t.y)) {
        case UiButton::Calendar:
            state = (state == AppState::CLOCK) ? AppState::CALENDAR : AppState::CLOCK;
            break;
        case UiButton::Settings:
            state = (state == AppState::SETTINGS) ? AppState::CLOCK : AppState::SETTINGS;
            break;
        case UiButton::Imu:
            settings.imuEnabled = !settings.imuEnabled;
            break;
        case UiButton::Restart:
            pomodoro.restart();      // vuelve al tiempo completo y arranca solo
            soundPlay(Sfx::Cancel);
            break;
        case UiButton::Stop:
            pomodoro.stop();         // descarta la sesión y libera el otro lado
            state = AppState::CLOCK;
            soundPlay(Sfx::Cancel);
            break;
        case UiButton::VolMinus:
        case UiButton::VolPlus: {
            const int d = (uiHit(t.x, t.y) == UiButton::VolPlus) ? 1 : -1;
            const int lv = settings.volumeLevel + d;
            if (lv >= 0 && lv < SOUND_LEVELS) {
                settings.volumeLevel = lv;
                soundSetLevel(lv);
                soundPlay(Sfx::Chime);   // muestra el nivel elegido (en silencio no suena)
                settingsChanged = true;
            }
            break;
        }
        case UiButton::BrightMinus:
        case UiButton::BrightPlus: {
            const int d = (uiHit(t.x, t.y) == UiButton::BrightPlus) ? 1 : -1;
            const int lv = settings.brightnessLevel + d;
            if (lv >= 0 && lv < BRIGHTNESS_LEVELS) {
                settings.brightnessLevel = lv;
                applyBrightness();   // se ve al instante
                settingsChanged = true;
            }
            break;
        }
        case UiButton::LowPower:     enterLowPower(); break;
        case UiButton::ExitLowPower: exitLowPower();  break;
        case UiButton::ShortMinus: if (settings.shortMin > 5)  { settings.shortMin -= POMODORO_STEP_MIN; settingsChanged = true; } break;
        case UiButton::ShortPlus:  if (settings.shortMin < 60) { settings.shortMin += POMODORO_STEP_MIN; settingsChanged = true; } break;
        case UiButton::LongMinus:  if (settings.longMin  > 10) { settings.longMin  -= POMODORO_STEP_MIN; settingsChanged = true; } break;
        case UiButton::LongPlus:   if (settings.longMin  < 90) { settings.longMin  += POMODORO_STEP_MIN; settingsChanged = true; } break;
        default: return;
    }
    if (settingsChanged) settingsSave(settings);
    needsRedraw = true;
}

void setup() {
    setCpuFrequencyMhz(NORMAL_CPU_MHZ);   // 160 MHz sobra para esta app y ahorra batería
    auto cfg = M5.config();
    CoreS3.begin(cfg);
    Serial.begin(115200);
    Serial.setTxTimeoutMs(0);   // no bloquear el loop si nadie lee el puerto serie
    settingsLoad(settings);   // minutos, volumen y brillo guardados
    uiInit();
    applyBrightness();
    soundInit(settings.volumeLevel);
    timeSyncInit();   // el RTC conserva la hora entre arranques; NTP la corrige en segundo plano
}

void loop() {
    CoreS3.update();

    handleTouch();

    if (state == AppState::LOW_POWER) { lowPowerLoop(); return; }

    const Orientation o = updateOrientation();

    if (o != lastOrientation) {
        lastOrientation = o;
        onOrientationChange(o);
    }

    const PomoState before = pomodoro.state();
    pomodoro.update();
    if (before == PomoState::Running && pomodoro.state() == PomoState::Finished) {
        soundPlay(Sfx::Chime);
        needsRedraw = true;
    }
    soundUpdate();
    timeSyncUpdate();

    // La interfaz solo rota en la pantalla del Pomodoro y en el aviso de bloqueo;
    // el resto del tiempo usa la base.
    const bool blocked = isBlocked(o);
    if (uiSetOrientation((state == AppState::POMODORO || blocked) ? o : Orientation::Base)) needsRedraw = true;

    if (needsRedraw || contentChanged()) {
        lastTick = millis();
        needsRedraw = false;
        uiDraw(state, settings, pomodoro, o, blocked);
    }

    delay(LOOP_DELAY_MS);
}
