// audio.cpp - the theme loops through an SDL audio callback; listen() looks at what's playing right now: its loudness,
// its bass (a simple low-pass filter), and beats (the bass jumping well above its own recent average).
#include "audio.h"
#include <SDL.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <vector>

#include "stb_vorbis.c"

namespace {
std::vector<short> pcm;              // interleaved stereo, 44.1 kHz
int channels = 2, rate = 44100;
std::atomic<size_t> pos{0};          // in samples (all channels)
std::atomic<bool> silent{false};
SDL_AudioDeviceID dev = 0;

// sound effects, made here (no files): each a short synthesised voice mixed over the theme
struct Voice { int type; float t; float phase; };
constexpr int MAXV = 8;
Voice voices[MAXV];
std::atomic<int> nvoices{0};
std::atomic<int> requests[3];                // Eat, Cut, Over: asked for by the game thread
unsigned noise = 12345;

float sfx(Voice& v, float dt) {               // one sample of a voice, or -2 when it's finished
    float f, amp, len;
    switch (v.type) {
    case 0:   // eat: a quick rising two-note chime
        len = 0.16f; f = v.t < 0.06f ? 988.f : 1480.f; amp = 0.22f * (1.f - v.t / len); break;
    case 1:   // cut: a falling zap with some grit
        len = 0.22f; f = 900.f - 3200.f * v.t; amp = 0.20f * (1.f - v.t / len); break;
    default:  // over: a long falling tone
        len = 0.9f; f = 440.f * std::pow(0.25f, v.t / len); amp = 0.25f * (1.f - v.t / len); break;
    }
    if (v.t >= len) return -2.f;
    v.phase += f * dt;
    v.phase -= std::floor(v.phase);
    float s = v.phase < 0.5f ? 1.f : -1.f;                                     // square, the arcade sound
    if (v.type == 0) s = std::sin(v.phase * 6.2831853f) * 0.7f + s * 0.3f;     // the chime rounder
    if (v.type == 1) { noise = noise * 1664525u + 1013904223u; s = s * 0.6f + ((noise >> 9) / 4194304.f - 1.f) * 0.4f; }
    v.t += dt;
    return s * amp;
}
float bass_avg = 0.f, since_beat = 1.f, energy_s = 0.f, bass_s = 0.f;

void callback(void*, Uint8* stream, int len) {
    short* out = (short*)stream;
    const int n = len / (int)sizeof(short);
    // start the effects the game asked for
    for (int type = 0; type < 3; ++type)
        for (int r = requests[type].exchange(0); r > 0 && nvoices < MAXV; --r) voices[nvoices++] = {type, 0.f, 0.f};
    const float dt = 1.f / rate;
    size_t p = pos.load();
    for (int i = 0; i < n; i += channels) {
        float fx = 0.f;
        for (int v = 0; v < nvoices; ) {
            const float s = sfx(voices[v], dt);
            if (s < -1.5f) { voices[v] = voices[--nvoices]; continue; }
            fx += s; ++v;
        }
        for (int c = 0; c < channels && i + c < n; ++c) {
            const float music = silent ? 0.f : pcm[p] / 32768.f * 0.8f;
            const float mix = music + (silent ? 0.f : fx);
            out[i + c] = (short)(std::clamp(mix, -1.f, 1.f) * 32767.f);
            if (++p >= pcm.size()) p = 0;             // round again
        }
    }
    pos.store(p);
}
}  // namespace

bool Audio::open(const std::string& path) {
    short* data = nullptr;
    const int frames = stb_vorbis_decode_filename(path.c_str(), &channels, &rate, &data);
    if (frames <= 0 || !data) return false;
    pcm.assign(data, data + (size_t)frames * channels);
    free(data);
    SDL_AudioSpec want{}, have{};
    want.freq = rate;
    want.format = AUDIO_S16SYS;
    want.channels = (Uint8)channels;
    want.samples = 1024;
    want.callback = callback;
    dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (!dev) return false;
    SDL_PauseAudioDevice(dev, 0);
    return true;
}

void Audio::close() {
    if (dev) SDL_CloseAudioDevice(dev);
    dev = 0;
}

void Audio::setMuted(bool m) { silent = m; }
void Audio::play(Sfx s) { if (dev) ++requests[(int)s]; }
bool Audio::muted() { return silent; }

Music Audio::listen(float dt) {
    Music m;
    if (!dev || pcm.empty()) return m;
    // the last ~23 ms of what's playing
    const size_t win = 1024 * (size_t)channels;
    const size_t end = pos.load();
    double sum = 0.0, low_sum = 0.0;
    float low = 0.f;
    for (size_t i = 0; i < win; i += channels) {
        const size_t k = (end + pcm.size() - win + i) % pcm.size();
        float s = 0.f;
        for (int c = 0; c < channels; ++c) s += pcm[(k + c) % pcm.size()] / 32768.f;
        s /= channels;
        low += 0.02f * (s - low);                       // a one-pole low-pass: the bass
        sum += s * s;
        low_sum += low * low;
    }
    const float n = (float)(win / channels);
    const float energy = std::sqrt((float)(sum / n)) * 3.f;
    const float bass = std::sqrt((float)(low_sum / n)) * 6.f;
    energy_s += (std::min(1.f, energy) - energy_s) * std::min(1.f, dt * 12.f);
    bass_s   += (std::min(1.f, bass) - bass_s) * std::min(1.f, dt * 16.f);
    since_beat += dt;
    m.beat = !silent && bass > bass_avg * 1.45f && bass > 0.08f && since_beat > 0.28f;
    if (m.beat) since_beat = 0.f;
    bass_avg += (bass - bass_avg) * std::min(1.f, dt * 1.5f);
    m.energy = silent ? 0.f : energy_s;
    m.bass = silent ? 0.f : bass_s;
    return m;
}
