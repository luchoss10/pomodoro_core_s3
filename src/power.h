#pragma once
#include <stdint.h>

// Tareas de energía que se hacen una vez tras el arranque (apagar la salida de 5 V).
// Llamar en cada loop.
void powerUpdate();

// true si el equipo está alimentado por USB (VBUS presente). Se cachea 2 s.
bool powerOnUsb();

// Espera `ms`: con batería, en sueño ligero (despierta antes si se toca la pantalla);
// con USB, con un delay normal de `usbDelayMs` (el sueño ligero corta la conexión USB
// y dificultaría flashear y usar el monitor serial).
void powerSleep(uint32_t ms, uint32_t usbDelayMs);

// Franja horaria del modo ahorro (índice para LP_BRIGHTNESS / LP_TEXT_COLOR): 0 día, 1 tarde, 2 noche.
int lowPowerPeriod(int hour);
