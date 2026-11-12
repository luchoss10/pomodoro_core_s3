#pragma once
#include <stdint.h>
#include "orientation.h"

enum class PomoState : uint8_t {
    Idle,       // sin sesión
    Running,    // contando
    Paused,     // en pausa (conserva el tiempo transcurrido)
    Finished,   // llegó a 00:00
};

// Una sola sesión a la vez, asociada al lado (Left/Right) que la inició.
class Pomodoro {
public:
    PomoState   state() const { return _state; }
    Orientation side() const  { return _side; }
    int         minutes() const { return _minutes; }

    void start(int minutes, Orientation side);  // nueva sesión, corriendo
    void pause();                               // conserva el progreso
    void resume();                              // continúa donde quedó
    void restart();                             // vuelve al tiempo completo y corre
    void stop();                                // descarta la sesión

    uint32_t remainingSec() const;
    float    remainingRatio() const;            // 1.0 (inicio) .. 0.0 (fin)
    void     update();                          // llamar en cada loop
private:
    uint32_t elapsedMs() const;

    PomoState   _state    = PomoState::Idle;
    Orientation _side     = Orientation::Base;
    int         _minutes  = 0;
    uint32_t    _accumMs  = 0;   // tiempo acumulado en tramos anteriores
    uint32_t    _startMs  = 0;   // inicio del tramo actual
};
