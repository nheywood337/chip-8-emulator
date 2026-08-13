#include "opcode.h"

#include <cstdint>

namespace opcode {

    Instruction decode(uint16_t raw_opcode) {
        Instruction instruction;
        uint8_t four_b_mask = 0xF; // MASK: 1111
        uint8_t eight_b_mask = 0xFF; // MASK: 1111 1111
        uint16_t twelve_b_mask = 0xFFF; // mask: 1111 1111 1111

        // 16 bits, 4 bits for each instruction member variable
        // n = no shift, y = 4 shift, x = 8 shift, f = 12 shift
        // use bitwise AND operator with our 1111 mask
        // ex: 0x00e0 = [0000 0000 1110 0000]
        //               ^15               ^0

        instruction.raw_opcode = raw_opcode;
        instruction.n = raw_opcode & four_b_mask;
        instruction.y = (raw_opcode >> 4) & four_b_mask;
        instruction.x = (raw_opcode >> 8) & four_b_mask;
        instruction.f = (raw_opcode >> 12) & four_b_mask;
        instruction.nn = raw_opcode & eight_b_mask;
        instruction.nnn = raw_opcode & twelve_b_mask;

        return instruction;
    }

}