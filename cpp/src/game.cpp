// game.cpp - Killer Stardust: steer the comet, eat the stardust to grow your tail; invaders fall in waves, faster
// each wave, and drop in on the beat of the theme. One through your tail cuts it there (everything behind is lost);
// one on your head, or running into your own tail, and it's over.
#include "game.h"
#include <algorithm>
#include <cmath>

ImU32 withAlpha(ImU32 c, float a) {
    const int al = (int)(((c >> IM_COL32_A_SHIFT) & 0xff) * std::clamp(a, 0.f, 1.f));
    return (c & ~IM_COL32_A_MASK) | ((ImU32)al << IM_COL32_A_SHIFT);
}

// a soft glow: a few widening, fading rounded rectangles
void glow(ImDrawList* dl, ImVec2 p0, ImVec2 p1, float r, ImU32 col, float size) {
    for (int i = 4; i >= 1; --i) {
        const float e = size * i / 4.f;
        dl->AddRectFilled({p0.x - e, p0.y - e}, {p1.x + e, p1.y + e}, withAlpha(col, 0.06f), r + e);
    }
}

static bool pressed(ImGuiKey a, ImGuiKey b) { return ImGui::IsKeyPressed(a, false) || ImGui::IsKeyPressed(b, false); }

Game::Game() {
    for (int i = 0; i < 220; ++i) stars_.push_back({rnd(0.f, 1.f), rnd(0.f, 1.f), rnd(0.f, 6.28f), rnd(0.5f, 2.5f)});
    reset();
}

void Game::reset() {
    score = 0; over = false; wave = 1;
    body_.clear();
    for (int i = 0; i < 3; ++i) body_.push_back({W / 2 - i, H / 2});
    dir_ = {1, 0};
    queue_.clear();
    length_ = 3;
    dust_.clear();
    for (int i = 0; i < 36; ++i) dust_.push_back({rndi(0, W - 1), rndi(0, H - 1)});
    inv_.clear();
    cuts_.clear();
    sparks_.clear();
    wave_t_ = 0.f; acc_ = 0.f;
    spawnWave();
}

// a burst of sparks flying out from a point, slowing and fading
void Game::burst(float x, float y, int n, float speed, ImU32 a, ImU32 b) {
    for (int i = 0; i < n; ++i) {
        const float ang = rnd(0.f, 6.2831853f), sp = speed * rnd(0.25f, 1.f), life = rnd(0.35f, 0.75f);
        sparks_.push_back({x, y, std::cos(ang) * sp, std::sin(ang) * sp, life, life, rnd(0.f, 1.f) < 0.3f ? b : a});
    }
}

float Game::fallSpeed() { return (2.2f + wave * 0.35f) * rnd(0.8f, 1.25f); }

void Game::spawnWave() {
    for (int i = 0; i < 3 + wave * 2; ++i) inv_.push_back({(float)rndi(0, W - 1), -rnd(1.f, 8.f), fallSpeed()});
}

void Game::step() {
    if (!queue_.empty()) { dir_ = queue_.front(); queue_.pop_front(); }
    const Cell h = {(body_.front().x + dir_.x + W) % W, (body_.front().y + dir_.y + H) % H};
    for (size_t i = 0; i + 1 < body_.size(); ++i)            // the very end moves away this step
        if (body_[i].x == h.x && body_[i].y == h.y) { over = true; return; }
    body_.push_front(h);
    for (auto& d : dust_)
        if (d.x == h.x && d.y == h.y) {
            score += 10; ++length_; ++ate;
            burst(d.x + 0.5f, d.y + 0.5f, 28, 7.f, Col::blue, Col::white);
            d = {rndi(0, W - 1), rndi(0, H - 1)};
        }
    while ((int)body_.size() > length_) body_.pop_back();
}

void Game::update(float dt, const Music& m) {
    auto turn = [&](int dx, int dy) {
        const Cell last = queue_.empty() ? dir_ : queue_.back();
        if ((last.x == -dx && last.y == -dy) || (last.x == dx && last.y == dy)) return;
        if (queue_.size() < 3) queue_.push_back({dx, dy});
    };
    if (pressed(ImGuiKey_LeftArrow,  ImGuiKey_A)) turn(-1, 0);
    if (pressed(ImGuiKey_RightArrow, ImGuiKey_D)) turn(1, 0);
    if (pressed(ImGuiKey_UpArrow,    ImGuiKey_W)) turn(0, -1);
    if (pressed(ImGuiKey_DownArrow,  ImGuiKey_S)) turn(0, 1);

    // waves on a clock, faster each time; on some beats of the theme an extra invader drops in
    wave_t_ += dt;
    if (wave_t_ > std::max(2.f, 6.f - wave * 0.4f)) { wave_t_ = 0.f; ++wave; spawnWave(); }
    if (m.beat && rnd(0.f, 1.f) < 0.45f) inv_.push_back({(float)rndi(0, W - 1), -1.f, fallSpeed() * 1.4f});

    // the comet: a cell a step, a little quicker as it grows
    const float interval = 0.11f * std::max(0.5f, 1.f - length_ * 0.004f);
    acc_ += dt;
    while (acc_ >= interval && !over) { acc_ -= interval; step(); }

    for (auto& v : inv_) {
        v.y += v.v * dt;
        if (v.y >= H) { v.y = -1.f; v.x = (float)rndi(0, W - 1); }
        const int vx = (int)std::lround(v.x), vy = (int)std::floor(v.y + 0.5f);
        if (vy < 0) continue;
        if (vx == body_.front().x && vy == body_.front().y) { over = true; return; }
        for (size_t i = 1; i < body_.size(); ++i) {
            if (body_[i].x == vx && body_[i].y == vy) {
                const int lost = (int)(body_.size() - i) * 10;          // every lost segment costs what it earned
                score = std::max(0, score - lost);
                cuts_.push_back({body_[i].x, body_[i].y, 0.9f, lost});
                for (size_t j = i; j < body_.size(); ++j)              // the lost tail breaks up into sparks
                    burst(body_[j].x + 0.5f, body_[j].y + 0.5f, 4, 4.f, Col::pink, Col::green);
                ++cut;
                body_.resize(i);
                length_ = std::max(3, (int)i);
                v.y = -1.f; v.x = (float)rndi(0, W - 1);
                break;
            }
        }
    }
    for (auto& s : sparks_) {
        s.x += s.vx * dt; s.y += s.vy * dt;
        const float drag = std::exp(-3.f * dt);
        s.vx *= drag; s.vy *= drag;
        s.life -= dt;
    }
    sparks_.erase(std::remove_if(sparks_.begin(), sparks_.end(), [](const Spark& s) { return s.life <= 0.f; }), sparks_.end());
    for (auto& c : cuts_) c.life -= dt;
    cuts_.erase(std::remove_if(cuts_.begin(), cuts_.end(), [](const Cut& c) { return c.life <= 0.f; }), cuts_.end());
}

void Game::drawStars(ImDrawList* dl, ImVec2 p0, ImVec2 p1, const Music& m, double t) {
    dl->AddRectFilled(p0, p1, Col::space);
    for (auto& s : stars_) {
        const float k = 0.3f + 0.7f * std::fabs(std::sin(s.t + (float)t * s.s * (1.f + m.energy * 2.f)));
        const ImVec2 q = {p0.x + s.x * (p1.x - p0.x), p0.y + s.y * (p1.y - p0.y)};
        const float r = 1.2f + (s.s > 2.2f ? 0.8f : 0.f);
        dl->AddRectFilled(q, {q.x + r, q.y + r}, withAlpha(Col::white, k));
    }
}

void Game::draw(ImDrawList* dl, ImVec2 p0, ImVec2 p1, const Music& m, double t) {
    drawStars(dl, p0, p1, m, t);
    const float top = 52.f;                                           // room for the score
    const float cs = std::floor(std::min((p1.x - p0.x - 24.f) / W, (p1.y - p0.y - top - 16.f) / H));
    const ImVec2 o = {std::floor((p0.x + p1.x - cs * W) * 0.5f), std::floor(p0.y + top + (p1.y - p0.y - top - cs * H) * 0.5f)};
    auto at = [&](float x, float y) { return ImVec2(o.x + x * cs, o.y + y * cs); };

    // the arena's edge glows with the bass
    const float edge = 3.f + m.bass * 10.f;                           // a glow along the edge only, not over the field
    for (int i = 4; i >= 1; --i) {
        const float e = edge * i / 4.f;
        dl->AddRect({at(0, 0).x - e, at(0, 0).y - e}, {at((float)W, (float)H).x + e, at((float)W, (float)H).y + e},
                    withAlpha(Col::pink, 0.10f), 6.f + e, 0, edge / 2.f);
    }
    dl->AddRect(at(0, 0), at((float)W, (float)H), withAlpha(Col::pink, 0.8f), 6.f, 0, 2.f);

    for (auto& d : dust_) {                                           // the stardust
        const ImVec2 c = at(d.x + 0.5f, d.y + 0.5f);
        const float r = cs * (0.17f + m.bass * 0.08f);
        for (int i = 3; i >= 1; --i) dl->AddCircleFilled(c, r + cs * 0.12f * i, withAlpha(Col::blue, 0.08f), 16);
        dl->AddCircleFilled(c, r, Col::blue, 16);
    }

    const int n = (int)body_.size();                                  // the comet: a white head, a fading pink tail
    for (int i = n - 1; i >= 0; --i) {
        const float fade = std::max(0.15f, 1.f - (float)i / (n + 2));
        const ImU32 col = i == 0 ? Col::white : withAlpha(Col::pink, fade);
        ImVec2 q0 = at((float)body_[i].x, (float)body_[i].y), q1 = at(body_[i].x + 1.f, body_[i].y + 1.f);
        const float in = cs * 0.1f;
        q0.x += in; q0.y += in; q1.x -= in; q1.y -= in;
        glow(dl, q0, q1, cs * 0.2f, i == 0 ? Col::pink : col, cs * (i == 0 ? 0.6f : 0.35f));
        dl->AddRectFilled(q0, q1, col, cs * 0.18f);
    }

    for (auto& v : inv_) {                                            // the invaders: a little ship each
        if (v.y < -0.9f) continue;
        const ImVec2 q = at(v.x, v.y);
        const ImVec2 b0 = {q.x + cs * 0.15f, q.y + cs * 0.25f}, b1 = {q.x + cs * 0.85f, q.y + cs * 0.7f};
        glow(dl, b0, b1, 2.f, Col::green, cs * 0.35f);
        dl->AddRectFilled(b0, b1, Col::green, 2.f);
        dl->AddRectFilled({q.x + cs * 0.05f, q.y + cs * 0.45f}, {q.x + cs * 0.2f, q.y + cs * 0.85f}, Col::green);
        dl->AddRectFilled({q.x + cs * 0.8f, q.y + cs * 0.45f}, {q.x + cs * 0.95f, q.y + cs * 0.85f}, Col::green);
    }

    for (auto& s : sparks_) {                                         // the sparks: a glow and a bright core
        const float k = s.life / s.max;
        const ImVec2 c = at(s.x, s.y);
        dl->AddCircleFilled(c, cs * (0.08f + 0.18f * k), withAlpha(s.col, 0.15f * k), 10);
        dl->AddCircleFilled(c, cs * (0.03f + 0.07f * k), withAlpha(s.col, k), 8);
    }

    for (auto& c : cuts_) {                                           // where the tail was just cut
        const float k = 1.f - c.life / 0.9f;
        if (k < 0.5f) {
            const float r = k * 2.f;
            dl->AddCircle(at(c.x + 0.5f, c.y + 0.5f), cs * (0.4f + r * 1.4f), withAlpha(Col::green, 1.f - r), 24, 3.f);
        }
        const std::string txt = "-" + std::to_string(c.lost);                 // the points lost, rising and fading
        const ImVec2 tp = at(c.x + 0.5f, c.y - k * 1.5f);
        const float fs = cs * 0.9f;
        const ImVec2 ts = ImGui::GetFont()->CalcTextSizeA(fs, FLT_MAX, 0.f, txt.c_str());
        dl->AddText(ImGui::GetFont(), fs, {tp.x - ts.x * 0.5f, tp.y - ts.y}, withAlpha(Col::pink, 1.f - k), txt.c_str());
    }
}
