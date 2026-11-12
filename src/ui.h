#pragma once
#include "orientation.h"
#include "pomodoro.h"
#include "settings.h"

enum class AppState { CLOCK, CALENDAR, POMODORO, SETTINGS, LOW_POWER };

enum class UiButton {
    None, Calendar, Settings, Imu,
    Restart, Stop,
    ShortMinus, ShortPlus, LongMinus, LongPlus,
    VolMinus, VolPlus,
    BrightMinus, BrightPlus,
    LowPower, ExitLowPower,
};

void uiInit();
// Rota la pantalla según la orientación. Devuelve true si cambió.
bool uiSetOrientation(Orientation o);
// Dibuja el estado actual y lo envía a la pantalla.
// `blocked`: se giró hacia un lado mientras hay un Pomodoro pausado en el otro.
void uiDraw(AppState state, const Settings& s, const Pomodoro& p, Orientation o, bool blocked);
// Botón bajo la coordenada táctil (según el último dibujo).
UiButton uiHit(int x, int y);
