#pragma once

#include <cstdint>
#include <array>
#include <string>
#include "chip8_specs.h"


class chip8 {
    public:
        chip8(const std::string& path);

        // Populates the memory buffer using the rom_path member variable
        bool load_rom_into_memory();

    private:
        std::string rom_path;
        std::array<uint8_t, MEMORY_SIZE> memory = {}; // fixed memory size 4096
};