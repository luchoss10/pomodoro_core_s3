#include "settings.h"
#include <Preferences.h>

static const char* NS = "settings";

static int clampInt(int v, int lo, int hi, int fallback) {
    return (v < lo || v > hi) ? fallback : v;
}

void settingsLoad(Settings& s) {
    Preferences prefs;
    if (!prefs.begin(NS, true)) return;   // solo lectura; si falla, quedan los valores por defecto
    // Cada valor se valida: si la memoria tiene algo fuera de rango, se usa el valor por defecto.
    s.shortMin        = clampInt(prefs.getInt("short", s.shortMin), 5, 60, POMODORO_LEFT_MIN);
    s.longMin         = clampInt(prefs.getInt("long",  s.longMin), 10, 90, POMODORO_RIGHT_MIN);
    s.volumeLevel     = clampInt(prefs.getInt("vol",   s.volumeLevel), 0, SOUND_LEVELS - 1, SOUND_DEFAULT_LEVEL);
    s.brightnessLevel = clampInt(prefs.getInt("bri",   s.brightnessLevel), 0, BRIGHTNESS_LEVELS - 1, BRIGHTNESS_DEFAULT_LEVEL);
    prefs.end();
}

void settingsSave(const Settings& s) {
    Preferences prefs;
    if (!prefs.begin(NS, false)) return;
    prefs.putInt("short", s.shortMin);
    prefs.putInt("long",  s.longMin);
    prefs.putInt("vol",   s.volumeLevel);
    prefs.putInt("bri",   s.brightnessLevel);
    prefs.end();
}
