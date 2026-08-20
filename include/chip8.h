#pragma once

#include "chip8_specs.h"
#include <cstdint>
#include <array>
#include <vector>
#include <stdexcept>
#include <string>

class chip8_error : public std::runtime_error {
    public:
        explicit chip8_error(const std::string& msg) : std::runtime_error(msg) {}
};

class chip8 {
    public:
        explicit chip8(const std::vector<uint8_t>& byte_stream);
        uint16_t fetch();
        uint16_t get_program_counter() const;

    private:
        std::array<uint8_t, MEMORY_SIZE> memory = {};       // fixed memory size 4096
        uint16_t program_counter = START_ADDRESS_OFFSET;
};