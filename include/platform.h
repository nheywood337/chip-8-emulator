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

class platform {
    public:
        platform(const std::string& title, int scale); // scale = screen pixels per CHIP-8 pixel
        ~platform();

        platform(const platform&) = delete;
        platform& operator=(const platform&) = delete;

        bool poll_events(); // false once the user wants to quit
        void render(const std::array<uint8_t, DISPLAY_WIDTH * DISPLAY_HEIGHT>& display);
        void set_beep(bool on);

        const std::array<uint8_t, 16>& get_keypad() const;

    private:
        // refcounted claim on SDL's process-global subsystems rather than
        // ownership, so two platforms can coexist. Hence no SDL_Quit().
        struct subsystems {
            subsystems();
            ~subsystems();
            subsystems(const subsystems&) = delete;
            subsystems& operator=(const subsystems&) = delete;
        };

        class beeper {
            public:
                beeper();
                ~beeper();
                beeper(const beeper&) = delete;
                beeper& operator=(const beeper&) = delete;

                void set(bool on);

            private:
                uint32_t device = 0; // SDL_AudioDeviceID, 0 = no device, stay quiet
                double phase = 0.0;  // survives across callbacks so start/stop doesn't click
                bool playing = false;
        };

        // order matters: SDL up before any handle, released after the last one
        subsystems sdl;
        std::unique_ptr<SDL_Window, sdl_window_deleter> window;
        std::unique_ptr<SDL_Renderer, sdl_renderer_deleter> renderer;
        std::unique_ptr<SDL_Texture, sdl_texture_deleter> texture;
        beeper sound;

        std::array<uint8_t, 16> keypad = {};
};
