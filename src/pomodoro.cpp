#include "pomodoro.h"
#include <Arduino.h>

void Pomodoro::start(int minutes, Orientation side) {
    _minutes  = minutes;
    _side     = side;
    _accumMs  = 0;
    _startMs  = millis();
    _state    = PomoState::Running;
}

void Pomodoro::pause() {
    if (_state != PomoState::Running) return;
    _accumMs += millis() - _startMs;
    _state = PomoState::Paused;
}

void Pomodoro::resume() {
    if (_state != PomoState::Paused) return;
    _startMs = millis();
    _state = PomoState::Running;
}

void Pomodoro::restart() {
    if (_state == PomoState::Idle) return;
    _accumMs = 0;
    _startMs = millis();
    _state   = PomoState::Running;
}

void Pomodoro::stop() {
    _state   = PomoState::Idle;
    _accumMs = 0;
}

uint32_t Pomodoro::elapsedMs() const {
    return _accumMs + (_state == PomoState::Running ? millis() - _startMs : 0);
}

uint32_t Pomodoro::remainingSec() const {
    if (_state == PomoState::Idle || _state == PomoState::Finished) return 0;
    const uint32_t total   = (uint32_t)_minutes * 60;
    const uint32_t elapsed = elapsedMs() / 1000;
    return elapsed >= total ? 0 : total - elapsed;
}

float Pomodoro::remainingRatio() const {
    if (_state == PomoState::Idle) return 1.0f;
    if (_state == PomoState::Finished || _minutes <= 0) return 0.0f;
    return remainingSec() / (_minutes * 60.0f);
}

void Pomodoro::update() {
    if (_state == PomoState::Running && remainingSec() == 0) _state = PomoState::Finished;
}
