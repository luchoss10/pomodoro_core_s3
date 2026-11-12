#include "orientation.h"
#include "config.h"
#include <M5CoreS3.h>
#include <math.h>

Orientation classify(float ax, Orientation current) {
    if (ax < -TILT_ENTER) return Orientation::Left;
    if (ax >  TILT_ENTER) return Orientation::Right;
    if (fabsf(ax) < TILT_EXIT) return Orientation::Base;
    return current;
}

Orientation updateOrientation() {
    static Orientation stable    = Orientation::Base;
    static Orientation candidate = Orientation::Base;
    static uint32_t    since     = 0;
    static uint32_t    lastLog   = 0;

    float ax, ay, az;
    if (!CoreS3.Imu.getAccel(&ax, &ay, &az)) return stable;

    if (Serial && millis() - lastLog >= 1000) {   // para calibrar desde el monitor serial
        lastLog = millis();
        Serial.printf("ax=%.2f ay=%.2f az=%.2f  orient=%d\n", ax, ay, az, (int)stable);
    }

    const Orientation now = classify(ax, stable);
    if (now != candidate) {
        candidate = now;
        since = millis();
    } else if (candidate != stable && millis() - since >= ORIENT_STABLE_MS) {
        stable = candidate;
    }
    return stable;
}
