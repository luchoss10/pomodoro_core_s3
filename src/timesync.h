#pragma once

// Si el RTC no tiene hora válida, la fija con la fecha de compilación.
void timeSyncInit();
// Sincroniza el RTC por NTP en segundo plano (no bloquea). Llamar en cada loop.
void timeSyncUpdate();
// Texto con el estado de la sincronización (para mostrar en pantalla).
const char* timeSyncStatus();
// Cancela una sincronización en curso y apaga el Wi-Fi (p. ej. al entrar en modo ahorro).
void timeSyncAbort();
// true mientras hay una sincronización en curso (Wi-Fi encendido): no dormir.
bool timeSyncBusy();
