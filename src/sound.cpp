#include "sound.h"
#include "config.h"
#include <M5CoreS3.h>

struct Note { uint16_t freq; uint16_t ms; };

static const Note NOTES_CHIME[]  = {{523, 90}, {659, 90}, {784, 180}};   // Do - Mi - Sol
static const Note NOTES_CANCEL[] = {{440, 90}, {349, 90}, {262, 180}};   // La - Fa - Do (baja)

static const Note* seq = nullptr;
static uint8_t     len = 0;
static uint8_t     idx = 0;

static void playNext() {
    if (idx >= len) { seq = nullptr; return; }
    const Note& n = seq[idx++];
    CoreS3.Speaker.tone(n.freq, n.ms);
}

static int currentLevel = SOUND_DEFAULT_LEVEL;

void soundSetLevel(int level) {
    if (level < 0) level = 0;
    if (level > SOUND_LEVELS - 1) level = SOUND_LEVELS - 1;
    currentLevel = level;
    CoreS3.Speaker.setVolume(SOUND_VOLUMES[level]);
}

void soundInit(int level) {
    soundSetLevel(level);
}

void soundPlay(Sfx s) {
    if (currentLevel == 0) return;   // silenciado
    if (s == Sfx::Chime)  { seq = NOTES_CHIME;  len = sizeof(NOTES_CHIME)  / sizeof(Note); }
    else                  { seq = NOTES_CANCEL; len = sizeof(NOTES_CANCEL) / sizeof(Note); }
    idx = 0;
    CoreS3.Speaker.stop();
    playNext();
}

void soundUpdate() {
    if (seq) {
        if (!CoreS3.Speaker.isPlaying()) playNext();
        return;
    }
    // Melodía terminada: apagar el altavoz para que el amplificador (AW88298) no consuma.
    // El siguiente tone() lo vuelve a encender solo.
    if (CoreS3.Speaker.isRunning() && !CoreS3.Speaker.isPlaying()) CoreS3.Speaker.end();
}

bool soundBusy() {
    return seq != nullptr || CoreS3.Speaker.isPlaying();
}
