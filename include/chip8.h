#pragma once

#include "chip8_specs.h"
#include <cstdint>
#include <array>
#include <vector>


class chip8 {
    public:
        chip8(const std::vector<uint8_t>& byte_stream);

    private:
        std::array<uint8_t, MEMORY_SIZE> memory = {}; // fixed memory size 4096
};