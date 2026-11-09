#pragma once
// audio.h - the theme: decoded once (stb_vorbis), looped through SDL, and listened to as it plays so the game can
// move with it (loudness, bass, beats).
#include "game.h"
#include <string>

namespace Audio {
    bool  open(const std::string& ogg_path);    // false: no sound (the game still runs)
    void  close();
    void  setMuted(bool muted);
    bool  muted();
    Music listen(float dt);                     // call once a frame
    enum Sfx { Eat, Cut, Over };
    void  play(Sfx s);                          // a sound effect, mixed over the theme
}
