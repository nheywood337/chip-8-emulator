#include "platform.h"
#include <SDL.h>
#include <iostream>

namespace {
    constexpr uint32_t COLOR_ON  = 0xE0F8D0FF;
    constexpr uint32_t COLOR_OFF = 0x0F180FFF;

    constexpr int      SAMPLE_RATE    = 44100;
    constexpr int      BEEP_HZ        = 440;
    constexpr int16_t  BEEP_AMPLITUDE = 3000;
    constexpr uint16_t AUDIO_BUFFER   = 512; // samples

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

    // userdata is the beeper's phase
    void audio_callback(void* userdata, uint8_t* stream, int len) {
        auto* phase = static_cast<double*>(userdata);
        auto* samples = reinterpret_cast<int16_t*>(stream);
        const int count = len / static_cast<int>(sizeof(int16_t));
        const double step = static_cast<double>(BEEP_HZ) / SAMPLE_RATE;

        for (int i = 0; i < count; ++i) {
            samples[i] = (*phase < 0.5) ? BEEP_AMPLITUDE : -BEEP_AMPLITUDE;
            *phase += step;
            if (*phase >= 1.0) {
                *phase -= 1.0;
            }
        }
    }
}

void sdl_window_deleter::operator()(SDL_Window* w) const     { SDL_DestroyWindow(w); }
void sdl_renderer_deleter::operator()(SDL_Renderer* r) const { SDL_DestroyRenderer(r); }
void sdl_texture_deleter::operator()(SDL_Texture* t) const   { SDL_DestroyTexture(t); }

platform::subsystems::subsystems() {
    sdl_check(SDL_InitSubSystem(SDL_INIT_VIDEO), "SDL_InitSubSystem(VIDEO)");

    // audio is optional - a box with no sound card should still run the ROM
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        std::cerr << "[platform] WARN: no audio subsystem (" << SDL_GetError() << "), running silent" << std::endl;
    }
}

platform::subsystems::~subsystems() {
    SDL_QuitSubSystem(SDL_INIT_AUDIO | SDL_INIT_VIDEO);
}

platform::beeper::beeper() {
    SDL_AudioSpec want = {};
    want.freq     = SAMPLE_RATE;
    want.format   = AUDIO_S16SYS;
    want.channels = 1;
    want.samples  = AUDIO_BUFFER;
    want.callback = audio_callback;
    want.userdata = &this->phase;

    SDL_AudioSpec got = {};
    // no allowed changes, so SDL converts instead of handing back a format
    // the callback isn't written for
    this->device = SDL_OpenAudioDevice(nullptr, 0, &want, &got, 0);

    if (this->device == 0) {
        std::cerr << "[platform] WARN: no audio device (" << SDL_GetError() << "), running silent" << std::endl;
    }
}

platform::beeper::~beeper() {
    if (this->device != 0) {
        SDL_CloseAudioDevice(this->device);
    }
}

void platform::beeper::set(bool on) {
    if (this->device == 0 || on == this->playing) {
        return;
    }

    SDL_PauseAudioDevice(this->device, on ? 0 : 1);
    this->playing = on;
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
}

platform::~platform() = default;

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

void platform::set_beep(bool on) {
    this->sound.set(on);
}

const std::array<uint8_t, 16>& platform::get_keypad() const {
    return this->keypad;
}
