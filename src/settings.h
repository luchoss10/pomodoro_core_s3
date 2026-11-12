#pragma once
#include "config.h"

struct Settings {
    int  shortMin   = POMODORO_LEFT_MIN;   // giro a la izquierda
    int  longMin    = POMODORO_RIGHT_MIN;  // giro a la derecha
    bool imuEnabled = true;                // false = IMU OFF (ignora el giro)
    int  volumeLevel = SOUND_DEFAULT_LEVEL; // 0 (silencio) .. SOUND_LEVELS-1
    int  brightnessLevel = BRIGHTNESS_DEFAULT_LEVEL; // 0 (20 %) .. BRIGHTNESS_LEVELS-1 (100 %)
};

// Persistencia en memoria no volátil (NVS): minutos de cada Pomodoro, volumen y brillo.
void settingsLoad(Settings& s);        // llamar al arrancar; valida cada valor
void settingsSave(const Settings& s);  // llamar cuando el usuario cambia un ajuste
