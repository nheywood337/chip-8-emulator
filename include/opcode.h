#pragma once

#include <cstdint>

namespace opcode {

    struct Instruction
    {
        uint8_t n = 0;
        uint8_t y = 0;
        uint8_t x = 0;
        uint8_t f = 0;
        uint8_t nn = 0;
        uint16_t nnn = 0;
        uint16_t raw_opcode = 0;
    };

    Instruction decode(uint16_t raw_opcode);
}