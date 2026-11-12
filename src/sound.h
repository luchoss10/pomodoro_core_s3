#pragma once

enum class Sfx {
    Chime,    // ascendente: inicio de un Pomodoro y sesión completada
    Cancel,   // descendente: reinicio y parada
};

void soundInit(int level);
void soundSetLevel(int level);   // 0 = silencio .. SOUND_LEVELS-1
void soundPlay(Sfx s);   // no bloquea: encola las notas
void soundUpdate();      // llamar en cada loop para avanzar la melodía
