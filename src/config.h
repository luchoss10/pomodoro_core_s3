#pragma once
#include <stdint.h>

// ---- Orientación (IMU) ----
// Eje X: gravedad < -TILT_ENTER => girado a la izquierda, > +TILT_ENTER => derecha.
// Entre ambos umbrales se mantiene el estado anterior (histéresis); por debajo
// de TILT_EXIT en valor absoluto se considera la posición base (cámara arriba).
constexpr float    TILT_ENTER       = 0.75f;
constexpr float    TILT_EXIT        = 0.35f;
constexpr uint32_t ORIENT_STABLE_MS = 300;    // tiempo que debe mantenerse una posición para aceptarla

// ---- Pomodoro (minutos por defecto, editables en Ajustes) ----
constexpr int POMODORO_LEFT_MIN  = 15;
constexpr int POMODORO_RIGHT_MIN = 30;
constexpr int POMODORO_STEP_MIN  = 5;

// ---- Rotación de pantalla ----
// Base = la posición con la cámara arriba (la de la v2: setRotation(0)).
// Solo se rota durante un Pomodoro. Si queda de lado o al revés, intercambia
// ROT_OFFSET_LEFT y ROT_OFFSET_RIGHT (1 <-> 3).
constexpr int ROT_BASE         = 0;
constexpr int ROT_OFFSET_LEFT  = 3;
constexpr int ROT_OFFSET_RIGHT = 1;

// ---- Sonido ----
// 5 niveles de volumen; el nivel 0 silencia. (El máximo es 255: el volumen es de 8 bits.)
constexpr int     SOUND_LEVELS        = 5;
constexpr int     SOUND_DEFAULT_LEVEL = 2;
constexpr uint8_t SOUND_VOLUMES[SOUND_LEVELS] = {0, 64, 128, 192, 255};

// ---- Hora (NTP, Bogotá UTC-5 sin horario de verano) ----
constexpr const char* TZ_BOGOTA   = "<-05>5";
constexpr const char* NTP_SERVER1 = "pool.ntp.org";
constexpr const char* NTP_SERVER2 = "time.google.com";
constexpr uint32_t WIFI_TIMEOUT_MS   = 15000;                 // máximo esperando conexión o respuesta NTP
constexpr uint32_t NTP_RESYNC_MS     = 12UL * 3600 * 1000;    // resincronizar cada 12 h
// Reintentos con espera creciente (x2) si falla la sincronización: tras el intento inicial,
// reintenta a los 30 s, 1 min y 2 min. Si el último también falla, no toca la hora y
// conserva la guardada hasta el siguiente ciclo (NTP_RESYNC_MS).
constexpr uint32_t NTP_BACKOFF_MS[]  = {30UL * 1000, 60UL * 1000, 120UL * 1000};
constexpr int      NTP_BACKOFF_COUNT = sizeof(NTP_BACKOFF_MS) / sizeof(NTP_BACKOFF_MS[0]);
constexpr int      RTC_MIN_VALID_YEAR = 2025;                 // un RTC con año menor se considera sin hora
constexpr uint32_t TIME_SAVE_MS = 30UL * 60 * 1000;           // guardar la última hora conocida en NVS cada 30 min

// ---- Pantalla y energía ----
constexpr int     BRIGHTNESS_LEVELS        = 5;
constexpr int     BRIGHTNESS_DEFAULT_LEVEL = 2;   // 60 % (valor por defecto; el usuario puede cambiarlo en Ajustes)
constexpr uint8_t BRIGHTNESS_VALUES[BRIGHTNESS_LEVELS] = {51, 102, 153, 204, 255};   // 20 %..100 %
constexpr uint8_t LOW_POWER_BRIGHTNESS = BRIGHTNESS_VALUES[0];   // el nivel más bajo (20 %) en modo ahorro
constexpr int     LOW_POWER_CPU_MHZ    = 80;      // mínimo compatible con Wi-Fi
constexpr int     NORMAL_CPU_MHZ       = 160;     // modo normal (el sistema arranca a 240)
constexpr uint32_t BATTERY_READ_MS     = 5000;    // cada cuánto se lee la batería

// ---- Ciclo principal ----
constexpr uint32_t LOOP_DELAY_MS      = 30;       // pausa del loop (el táctil responde en < ~50 ms)
constexpr uint32_t IDLE_REDRAW_MS     = 30000;    // calendario y ajustes: refresco de respaldo
