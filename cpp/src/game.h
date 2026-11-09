#pragma once
// game.h - Killer Stardust's rules and drawing: the comet, the stardust, the invaders, the starfield.
#include <imgui.h>
#include <deque>
#include <random>
#include <string>
#include <vector>

struct Music {                 // what the theme is doing right now (audio.cpp)
    float energy = 0.f;        // loudness 0..1
    float bass   = 0.f;        // low end 0..1
    bool  beat   = false;      // a beat started this frame
};

class Game {
public:
    Game();
    void reset();
    void update(float dt, const Music& m);           // reads the keyboard through ImGui
    void draw(ImDrawList* dl, ImVec2 p0, ImVec2 p1, const Music& m, double t);
    void drawStars(ImDrawList* dl, ImVec2 p0, ImVec2 p1, const Music& m, double t);   // the splash uses them too

    int  score = 0;
    int  wave  = 1;
    bool over  = false;
    // what happened this frame, for the sound effects (main.cpp clears them after playing)
    int  ate = 0, cut = 0;

private:
    struct Cell { int x, y; };
    struct Inv  { float x, y, v; };
    struct Cut  { int x, y; float life; int lost; };   // lost: the points taken, shown rising from the cut
    struct Star { float x, y, t, s; };
    struct Spark { float x, y, vx, vy, life, max; ImU32 col; };   // particles, in grid cells

    std::vector<Spark> sparks_;
    void burst(float x, float y, int n, float speed, ImU32 a, ImU32 b);

    int W = 40, H = 24, length_ = 3;
    std::deque<Cell> body_, queue_;
    Cell dir_{1, 0};
    std::vector<Cell> dust_;
    std::vector<Inv>  inv_;
    std::vector<Cut>  cuts_;
    std::vector<Star> stars_;
    float acc_ = 0.f, wave_t_ = 0.f;
    std::mt19937 rng_{std::random_device{}()};

    float rnd(float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng_); }
    int   rndi(int a, int b)    { return std::uniform_int_distribution<int>(a, b)(rng_); }
    float fallSpeed();
    void  spawnWave();
    void  step();
};

// the Cursed colours
namespace Col {
    constexpr ImU32 pink    = IM_COL32(240, 136, 233, 255);   // #f088e9: the comet
    constexpr ImU32 blue    = IM_COL32(52, 169, 227, 255);    // #34a9e3: the stardust
    constexpr ImU32 green   = IM_COL32(57, 255, 106, 255);    // #39ff6a: the invaders
    constexpr ImU32 white   = IM_COL32(255, 255, 255, 255);
    constexpr ImU32 space   = IM_COL32(2, 6, 17, 255);
}
ImU32 withAlpha(ImU32 c, float a);
void  glow(ImDrawList* dl, ImVec2 p0, ImVec2 p1, float r, ImU32 col, float size);
