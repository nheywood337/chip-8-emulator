#pragma once

#include "chip8_specs.h"
#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>

// forward declared so SDL headers stay inside platform.cpp
struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

class platform_error : public std::runtime_error {
    public:
        explicit platform_error(const std::string& msg) : std::runtime_error(msg) {}
};

// window, renderer and input. The interpreter core doesn't know this exists.
class platform {
    public:
        platform(const std::string& title, int scale); // scale = screen pixels per CHIP-8 pixel
        ~platform();

        // owns raw SDL handles, so no copying
        platform(const platform&) = delete;
        platform& operator=(const platform&) = delete;

        bool poll_events();  // returns false once the user wants to quit
        void render(const std::array<uint8_t, DISPLAY_WIDTH * DISPLAY_HEIGHT>& display);

        const std::array<uint8_t, 16>& get_keypad() const;

    private:
        SDL_Window* window = nullptr;
        SDL_Renderer* renderer = nullptr;
        SDL_Texture* texture = nullptr;

        std::array<uint8_t, 16> keypad = {};

        // one packed RGBA word per pixel, reused every frame
        std::array<uint32_t, DISPLAY_WIDTH * DISPLAY_HEIGHT> pixels = {};
};
