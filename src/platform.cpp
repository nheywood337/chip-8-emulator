#include "platform.h"
#include <SDL.h>

namespace {
    constexpr uint32_t COLOR_ON  = 0xE0F8D0FF;
    constexpr uint32_t COLOR_OFF = 0x0F180FFF;

    // square wave, deliberately quiet
    constexpr int AUDIO_SAMPLE_RATE = 44100;
    constexpr int BEEP_FREQUENCY    = 440;   // Hz
    constexpr int BEEP_AMPLITUDE    = 3000;  // of a 32767 max
    constexpr int BEEP_PERIOD       = AUDIO_SAMPLE_RATE / BEEP_FREQUENCY; // samples

    // runs on SDL's audio thread. phase lives in platform so the wave carries on
    // where it stopped - restarting it every beep clicks.
    void audio_callback(void* userdata, uint8_t* stream, int len) {
        auto* phase = static_cast<int*>(userdata);
        auto* samples = reinterpret_cast<int16_t*>(stream);
        const int sample_count = len / static_cast<int>(sizeof(int16_t));

        for (int i = 0; i < sample_count; ++i) {
            samples[i] = static_cast<int16_t>(*phase < BEEP_PERIOD / 2 ? BEEP_AMPLITUDE
                                                                       : -BEEP_AMPLITUDE);
            *phase = (*phase + 1) % BEEP_PERIOD;
        }
    }

    void sdl_check(int result, const char* what) {
        if (result != 0) {
            throw platform_error(std::string("[platform] ERROR: ") + what + ": " + SDL_GetError());
        }
    }

    // keypad sits on the left of the keyboard:
    //   1 2 3 4  ->  1 2 3 C
    //   Q W E R  ->  4 5 6 D
    //   A S D F  ->  7 8 9 E
    //   Z X C V  ->  A 0 B F
    int key_to_chip8(SDL_Keycode key) {
        switch (key) {
            case SDLK_1: return 0x1;
            case SDLK_2: return 0x2;
            case SDLK_3: return 0x3;
            case SDLK_4: return 0xC;
            case SDLK_q: return 0x4;
            case SDLK_w: return 0x5;
            case SDLK_e: return 0x6;
            case SDLK_r: return 0xD;
            case SDLK_a: return 0x7;
            case SDLK_s: return 0x8;
            case SDLK_d: return 0x9;
            case SDLK_f: return 0xE;
            case SDLK_z: return 0xA;
            case SDLK_x: return 0x0;
            case SDLK_c: return 0xB;
            case SDLK_v: return 0xF;
            default:     return -1; // not a keypad key
        }
    }
}

void sdl_window_deleter::operator()(SDL_Window* w) const     { SDL_DestroyWindow(w); }
void sdl_renderer_deleter::operator()(SDL_Renderer* r) const { SDL_DestroyRenderer(r); }
void sdl_texture_deleter::operator()(SDL_Texture* t) const   { SDL_DestroyTexture(t); }

platform::subsystems::subsystems() {
    sdl_check(SDL_InitSubSystem(SDL_INIT_VIDEO), "SDL_InitSubSystem(VIDEO)");

    // no sound card shouldn't stop a ROM from running
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        SDL_Log("[platform] WARNING: audio unavailable, running silently: %s", SDL_GetError());
    }
}

platform::subsystems::~subsystems() {
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

platform::platform(const std::string& title, int scale) {
    if (scale < 1) {
        throw platform_error("[platform] ERROR: scale must be at least 1");
    }

    // nearest-neighbour, or the pixels come out blurry. Must be set
    // before the renderer is created.
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    this->window.reset(SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        static_cast<int>(DISPLAY_WIDTH) * scale,
        static_cast<int>(DISPLAY_HEIGHT) * scale,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    ));

    if (this->window == nullptr) {
        throw platform_error(std::string("[platform] ERROR: SDL_CreateWindow: ") + SDL_GetError());
    }

    // no vsync on purpose - main.cpp already paces the loop at 60 Hz
    this->renderer.reset(SDL_CreateRenderer(this->window.get(), -1, 0));

    if (this->renderer == nullptr) {
        throw platform_error(std::string("[platform] ERROR: SDL_CreateRenderer: ") + SDL_GetError());
    }

    sdl_check(SDL_RenderSetLogicalSize(this->renderer.get(),
                                       static_cast<int>(DISPLAY_WIDTH),
                                       static_cast<int>(DISPLAY_HEIGHT)),
              "SDL_RenderSetLogicalSize");
    sdl_check(SDL_RenderSetIntegerScale(this->renderer.get(), SDL_TRUE), "SDL_RenderSetIntegerScale");
    sdl_check(SDL_SetRenderDrawColor(this->renderer.get(), 0, 0, 0, 255), "SDL_SetRenderDrawColor");

    this->texture.reset(SDL_CreateTexture(
        this->renderer.get(),
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        static_cast<int>(DISPLAY_WIDTH),
        static_cast<int>(DISPLAY_HEIGHT)
    ));

    if (this->texture == nullptr) {
        throw platform_error(std::string("[platform] ERROR: SDL_CreateTexture: ") + SDL_GetError());
    }

    // no obtained spec, so SDL converts for us and the callback can always
    // assume mono 16-bit
    SDL_AudioSpec want = {};
    want.freq     = AUDIO_SAMPLE_RATE;
    want.format   = AUDIO_S16SYS;
    want.channels = 1;
    want.samples  = 512; // ~12 ms, short enough to keep up with the timer
    want.callback = audio_callback;
    want.userdata = &this->audio_phase;

    // opens paused - set_beeping() starts it
    this->audio_device = SDL_OpenAudioDevice(nullptr, 0, &want, nullptr, 0);

    if (this->audio_device == 0) {
        SDL_Log("[platform] WARNING: no beep: %s", SDL_GetError());
    }
}

platform::~platform() {
    if (this->audio_device != 0) {
        SDL_CloseAudioDevice(this->audio_device);
    }
}

bool platform::poll_events() {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                return false;

            case SDL_KEYDOWN: {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    return false;
                }
                const int key = key_to_chip8(event.key.keysym.sym);
                if (key >= 0) {
                    this->keypad.at(static_cast<size_t>(key)) = 1;
                }
                break;
            }

            case SDL_KEYUP: {
                const int key = key_to_chip8(event.key.keysym.sym);
                if (key >= 0) {
                    this->keypad.at(static_cast<size_t>(key)) = 0;
                }
                break;
            }

            default:
                break;
        }
    }

    return true;
}

void platform::render(const std::array<uint8_t, DISPLAY_WIDTH * DISPLAY_HEIGHT>& display) {
    void* raw = nullptr;
    int pitch = 0;

    sdl_check(SDL_LockTexture(this->texture.get(), nullptr, &raw, &pitch), "SDL_LockTexture");

    auto* bytes = static_cast<uint8_t*>(raw);
    for (size_t y = 0; y < DISPLAY_HEIGHT; ++y) {
        auto* row = reinterpret_cast<uint32_t*>(bytes + y * static_cast<size_t>(pitch));
        for (size_t x = 0; x < DISPLAY_WIDTH; ++x) {
            row[x] = display[y * DISPLAY_WIDTH + x] ? COLOR_ON : COLOR_OFF;
        }
    }

    SDL_UnlockTexture(this->texture.get());

    sdl_check(SDL_RenderClear(this->renderer.get()), "SDL_RenderClear");
    sdl_check(SDL_RenderCopy(this->renderer.get(), this->texture.get(), nullptr, nullptr), "SDL_RenderCopy");
    SDL_RenderPresent(this->renderer.get());
}

void platform::set_beeping(bool on) {
    if (this->audio_device == 0 || on == this->beeping) {
        return;
    }

    SDL_PauseAudioDevice(this->audio_device, on ? 0 : 1);
    this->beeping = on;
}

const std::array<uint8_t, 16>& platform::get_keypad() const {
    return this->keypad;
}
