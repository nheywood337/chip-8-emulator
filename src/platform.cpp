#include "platform.h"
#include <SDL.h>

namespace {
    // 0xRRGGBBAA to match SDL_PIXELFORMAT_RGBA8888
    constexpr uint32_t COLOR_ON  = 0xE0F8D0FF;
    constexpr uint32_t COLOR_OFF = 0x0F180FFF;

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

platform::platform(const std::string& title, int scale) {
    if (scale < 1) {
        throw platform_error("[platform] ERROR: scale must be at least 1");
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        throw platform_error(std::string("[platform] ERROR: SDL_Init failed: ") + SDL_GetError());
    }

    // nearest-neighbour, otherwise the pixels come out blurry.
    // Has to be set before the renderer is created.
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    window = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        static_cast<int>(DISPLAY_WIDTH) * scale,
        static_cast<int>(DISPLAY_HEIGHT) * scale,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (window == nullptr) {
        SDL_Quit();
        throw platform_error(std::string("[platform] ERROR: SDL_CreateWindow failed: ") + SDL_GetError());
    }

    renderer = SDL_CreateRenderer(window, -1, 0);

    if (renderer == nullptr) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        throw platform_error(std::string("[platform] ERROR: SDL_CreateRenderer failed: ") + SDL_GetError());
    }

    // draw in 64x32 coords and let SDL scale up
    SDL_RenderSetLogicalSize(renderer, static_cast<int>(DISPLAY_WIDTH), static_cast<int>(DISPLAY_HEIGHT));
    SDL_RenderSetIntegerScale(renderer, SDL_TRUE);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        static_cast<int>(DISPLAY_WIDTH),
        static_cast<int>(DISPLAY_HEIGHT)
    );

    if (texture == nullptr) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        throw platform_error(std::string("[platform] ERROR: SDL_CreateTexture failed: ") + SDL_GetError());
    }
}

platform::~platform() {
    // reverse order of creation
    if (texture  != nullptr) SDL_DestroyTexture(texture);
    if (renderer != nullptr) SDL_DestroyRenderer(renderer);
    if (window   != nullptr) SDL_DestroyWindow(window);
    SDL_Quit();
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
    // expand the emulator's 1-byte-per-pixel buffer into colours
    for (size_t i = 0; i < display.size(); ++i) {
        this->pixels[i] = display[i] ? COLOR_ON : COLOR_OFF;
    }

    SDL_UpdateTexture(
        this->texture,
        nullptr,
        this->pixels.data(),
        static_cast<int>(DISPLAY_WIDTH * sizeof(uint32_t))
    );

    SDL_RenderClear(this->renderer);
    SDL_RenderCopy(this->renderer, this->texture, nullptr, nullptr);
    SDL_RenderPresent(this->renderer);
}

const std::array<uint8_t, 16>& platform::get_keypad() const {
    return this->keypad;
}
