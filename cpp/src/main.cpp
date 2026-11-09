// main.cpp - Killer Stardust: the window, the splash (the Killer Stardust and Cursed logos), the game, the HUD.
//
//   any key   start (on the splash)          Arrows / WASD   steer
//   P         pause                           Space / R       play again after a game over
//   M         music on / off                  F11             fullscreen
//   Esc       back to the splash (from the splash: quit)
#include "audio.h"
#include "game.h"

#include <SDL.h>
#include <SDL_opengl.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <cmath>
#include <fstream>
#include <string>

namespace {

std::string base;                                   // the folder the exe is in
std::string asset(const char* name) { return base + "assets/" + name; }

struct Texture { GLuint id = 0; int w = 0, h = 0; };

Texture loadTexture(const std::string& path) {
    Texture t;
    int n = 0;
    unsigned char* px = stbi_load(path.c_str(), &t.w, &t.h, &n, 4);
    if (!px) return t;
    glGenTextures(1, &t.id);
    glBindTexture(GL_TEXTURE_2D, t.id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t.w, t.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    stbi_image_free(px);
    return t;
}

// the best score, kept in the user's settings folder
std::string bestFile() {
    char* p = SDL_GetPrefPath("Cursed Entertainment", "Killer Stardust");
    std::string s = p ? std::string(p) + "best.txt" : "best.txt";
    SDL_free(p);
    return s;
}
int loadBest() { std::ifstream f(bestFile()); int b = 0; f >> b; return b; }
void saveBest(int b) { std::ofstream f(bestFile()); f << b; }

void centredText(ImDrawList* dl, ImFont* font, float size, float cx, float y, ImU32 col, const std::string& s) {
    const ImVec2 sz = font->CalcTextSizeA(size, FLT_MAX, 0.f, s.c_str());
    dl->AddText(font, size, {cx - sz.x * 0.5f, y}, col, s.c_str());
}

void image(ImDrawList* dl, const Texture& t, float cx, float y, float width) {
    if (!t.id) return;
    const float h = width * t.h / t.w;
    dl->AddImage((ImTextureID)(intptr_t)t.id, {cx - width * 0.5f, y}, {cx + width * 0.5f, y + h});
}

}  // namespace

int main(int, char**) {
    char* bp = SDL_GetBasePath();
    base = bp ? bp : "";
    SDL_free(bp);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) return 1;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    SDL_Window* win = SDL_CreateWindow("Killer Stardust", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 800,
                                       SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!win) return 1;
    SDL_GLContext gl = SDL_GL_CreateContext(win);
    SDL_GL_MakeCurrent(win, gl);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    // a bold sans font when the system has one, else ImGui's own
    ImFont* font = nullptr;
    for (const char* f : {"C:/Windows/Fonts/arialbd.ttf", "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
                          "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf"}) {
        std::ifstream test(f);
        if (test.good()) { font = io.Fonts->AddFontFromFileTTF(f, 64.f); break; }
    }
    if (!font) font = io.Fonts->AddFontDefault();
    ImGui_ImplSDL2_InitForOpenGL(win, gl);
    ImGui_ImplOpenGL3_Init("#version 330");

    const Texture logo = loadTexture(asset("kdlogo.png"));
    const Texture cursed = loadTexture(asset("cursed-logo.png"));
    if (SDL_Surface* icon = SDL_LoadBMP(asset("icon.bmp").c_str())) { SDL_SetWindowIcon(win, icon); SDL_FreeSurface(icon); }
    const bool sound = Audio::open(asset("theme.ogg"));

    enum class State { Splash, Playing, Paused, Over } state = State::Splash;
    Game game;
    int best = loadBest();
    bool fullscreen = false, quit = false;
    Uint64 last = SDL_GetPerformanceCounter();

    while (!quit) {
        SDL_Event e;
        bool any_key = false;
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL2_ProcessEvent(&e);
            if (e.type == SDL_QUIT) quit = true;
            if (e.type == SDL_MOUSEBUTTONDOWN) any_key = true;
            if (e.type != SDL_KEYDOWN || e.key.repeat) continue;
            const SDL_Keycode k = e.key.keysym.sym;
            any_key = true;
            if (k == SDLK_F11 || (k == SDLK_RETURN && (e.key.keysym.mod & KMOD_ALT))) {
                fullscreen = !fullscreen;
                SDL_SetWindowFullscreen(win, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                any_key = false;
            } else if (k == SDLK_m) {
                Audio::setMuted(!Audio::muted());
                any_key = false;
            } else if (k == SDLK_ESCAPE) {
                if (state == State::Splash) quit = true; else state = State::Splash;
                any_key = false;
            } else if (k == SDLK_p && (state == State::Playing || state == State::Paused)) {
                state = state == State::Playing ? State::Paused : State::Playing;
                any_key = false;
            } else if ((k == SDLK_SPACE || k == SDLK_r) && state == State::Over) {
                game.reset();
                state = State::Playing;
                any_key = false;
            }
        }
        if (state == State::Splash && any_key) { game.reset(); state = State::Playing; }

        const Uint64 now = SDL_GetPerformanceCounter();
        const float dt = std::min(0.05f, (float)(now - last) / (float)SDL_GetPerformanceFrequency());
        last = now;
        const double t = SDL_GetTicks() / 1000.0;
        const Music music = Audio::listen(dt);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        if (state == State::Playing) {
            game.update(dt, music);
            if (game.ate) Audio::play(Audio::Eat);
            if (game.cut) Audio::play(Audio::Cut);
            game.ate = game.cut = 0;
            if (game.over) {
                Audio::play(Audio::Over);
                state = State::Over;
                if (game.score > best) { best = game.score; saveBest(best); }
            }
        }

        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        const ImVec2 p0 = {0, 0}, p1 = io.DisplaySize;
        const float cx = p1.x * 0.5f;
        if (state == State::Splash) {
            game.drawStars(dl, p0, p1, music, t);
            const float lw = std::min(640.f, p1.x * 0.7f);
            const float ly = p1.y * 0.16f;
            image(dl, logo, cx, ly, lw);
            const float lh = logo.id ? lw * logo.h / logo.w : 0.f;
            const float cw = std::min(340.f, p1.x * 0.4f);
            image(dl, cursed, cx, ly + lh + 30.f, cw);
            const float ch = cursed.id ? cw * cursed.h / cursed.w : 0.f;
            centredText(dl, font, 20.f, cx, ly + lh + ch + 44.f, IM_COL32(194, 194, 194, 255), "\xC2\xA9 Cursed Entertainment 2026");
            if (std::fmod(t, 1.2) < 0.75)
                centredText(dl, font, 28.f, cx, ly + lh + ch + 110.f, Col::pink, "PRESS ANY KEY");
            centredText(dl, font, 16.f, cx, p1.y - 40.f, IM_COL32(134, 134, 134, 255),
                        "Arrows / WASD steer  \xC2\xB7  P pause  \xC2\xB7  M music  \xC2\xB7  F11 fullscreen  \xC2\xB7  Esc quit");
        } else {
            game.draw(dl, p0, p1, music, t);
            const std::string hud = "SCORE " + std::to_string(game.score) + "     WAVE " + std::to_string(game.wave) +
                                    "     BEST " + std::to_string(std::max(best, game.score));
            dl->AddText(font, 26.f, {20.f, 14.f}, Col::pink, hud.c_str());
            const char* right = sound ? (Audio::muted() ? "M: music off" : "M: music on") : "no sound device";
            const ImVec2 rs = font->CalcTextSizeA(16.f, FLT_MAX, 0.f, right);
            dl->AddText(font, 16.f, {p1.x - rs.x - 20.f, 20.f}, IM_COL32(194, 194, 194, 255), right);
            if (state != State::Playing) {
                dl->AddRectFilled(p0, p1, IM_COL32(2, 6, 17, 160));
                const bool over = state == State::Over;
                centredText(dl, font, 72.f, cx, p1.y * 0.36f, Col::pink, over ? "GAME OVER" : "PAUSED");
                centredText(dl, font, 24.f, cx, p1.y * 0.36f + 96.f, Col::white,
                            over ? "Score " + std::to_string(game.score) + "   \xC2\xB7   Space or R to play again   \xC2\xB7   Esc for the menu"
                                 : std::string("P to carry on"));
            }
        }

        ImGui::Render();
        int w, h;
        SDL_GL_GetDrawableSize(win, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.008f, 0.024f, 0.067f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(win);
    }

    Audio::close();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
