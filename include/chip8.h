#pragma once

#include <cstdint>
#include <array>
#include <string>
#include "chip8_specs.h"


class chip8 {
    public:
        chip8(const std::string& path);
        // populates memory buffer
        bool load_rom_into_memory();

    private:
        std::string rom_path;
        std::array<uint8_t, MEMORY_SIZE> memory = {}; // fixed memory size 4096
};