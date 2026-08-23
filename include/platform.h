#pragma once

#include "chip8_specs.h"
#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

class platform_error : public std::runtime_error {
    public:
        explicit platform_error(const std::string& msg) : std::runtime_error(msg) {}
};

// defined in platform.cpp, so ~platform() has to stay out of line
struct sdl_window_deleter   { void operator()(SDL_Window* w) const; };
struct sdl_renderer_deleter { void operator()(SDL_Renderer* r) const; };
struct sdl_texture_deleter  { void operator()(SDL_Texture* t) const; };

using window_ptr   = std::unique_ptr<SDL_Window, sdl_window_deleter>;
using renderer_ptr = std::unique_ptr<SDL_Renderer, sdl_renderer_deleter>;
using texture_ptr  = std::unique_ptr<SDL_Texture, sdl_texture_deleter>;

static_assert(sizeof(window_ptr) == sizeof(SDL_Window*), "deleter is costing a word");

class platform {
    public:
        platform(const std::string& title, int scale);
        ~platform();

        platform(const platform&) = delete;
        platform& operator=(const platform&) = delete;

        bool poll_events();
        void render(const std::array<uint8_t, DISPLAY_WIDTH * DISPLAY_HEIGHT>& display);

        const std::array<uint8_t, 16>& get_keypad() const;

    private:
        struct subsystems {
            subsystems();
            ~subsystems();
            subsystems(const subsystems&) = delete;
            subsystems& operator=(const subsystems&) = delete;
        };

        subsystems sdl;
        window_ptr window;
        renderer_ptr renderer;
        texture_ptr texture;

        std::array<uint8_t, 16> keypad = {};
};
