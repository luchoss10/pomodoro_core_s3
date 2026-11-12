#include "power.h"
#include "config.h"
#include <M5CoreS3.h>

static bool     extOutputHandled = false;
static bool     usbCache         = true;
static uint32_t lastUsbCheck     = 0;

void powerUpdate() {
    // La librería deja encendido el elevador de 5 V (Grove/bus) que esta app no usa.
    // Solo se apaga con batería presente (sin batería la librería lo desaconseja).
    // Se espera unos segundos tras el arranque para que la presencia de batería esté decidida.
    if (extOutputHandled || millis() < 3000) return;
    extOutputHandled = true;
    if (M5.Power.getBatteryPresence() == m5::battery_presence_t::present) {
        const bool ok = M5.Power.setExtOutput(false);
        if (Serial) Serial.printf("[energia] salida 5V %s\n", ok ? "apagada" : "no se pudo apagar");
    }
}

bool powerOnUsb() {
    if (lastUsbCheck == 0 || millis() - lastUsbCheck >= 2000) {
        lastUsbCheck = millis() | 1;   // nunca 0
        usbCache = M5.Power.getVBUSVoltage() > 4000;
    }
    return usbCache;
}

void powerSleep(uint32_t ms, uint32_t usbDelayMs) {
    if (powerOnUsb()) { delay(usbDelayMs); return; }
    M5.Power.lightSleep((uint64_t)ms * 1000ULL, true);   // true = despertar al tocar la pantalla
}

int lowPowerPeriod(int hour) {
    if (hour >= LP_DAY_START_H && hour < LP_EVENING_START_H)   return 0;   // día
    if (hour >= LP_EVENING_START_H && hour < LP_NIGHT_START_H) return 1;   // tarde
    return 2;                                                              // noche
}
