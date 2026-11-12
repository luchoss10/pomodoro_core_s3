#pragma once
#include <stdint.h>

enum class Orientation : uint8_t {
    Base,    // posición base: cámara arriba
    Left,    // girado a la izquierda (gravedad en X negativo)
    Right,   // girado a la derecha (gravedad en X positivo)
};

// Orientación según el eje X, con histéresis: entre TILT_EXIT y TILT_ENTER
// conserva la orientación `current`.
Orientation classify(float ax, Orientation current);

// Lee el IMU y devuelve la orientación ESTABLE (filtrada por tiempo).
Orientation updateOrientation();
