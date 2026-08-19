#include "opcode.h"
#include <cstdint>

namespace opcode {

    Instruction decode(uint16_t raw_opcode) {
        Instruction instruction;
        const uint8_t four_b_mask = 0xF;          // MASK: 1111
        const uint8_t eight_b_mask = 0xFF;        // MASK: 1111 1111
        const uint16_t twelve_b_mask = 0xFFF;     // MASK: 1111 1111 1111

        // 16 bits, 4 bits for each instruction member variable
        // n = no shift, y = 4 shift, x = 8 shift, f = 12 shift
        // use bitwise AND operator with our 1111 mask
        // ex: 0x00e0 = [0000 0000 1110 0000]
        //               ^15               ^0

        instruction.raw_opcode = raw_opcode;
        instruction.n   = raw_opcode & four_b_mask;
        instruction.y   = (raw_opcode >> 4) & four_b_mask;
        instruction.x   = (raw_opcode >> 8) & four_b_mask;
        instruction.f   = (raw_opcode >> 12) & four_b_mask;
        instruction.nn  = raw_opcode & eight_b_mask;
        instruction.nnn = raw_opcode & twelve_b_mask;

        // ex: [0x00E0]
        //   f = 0  :  dictates call type
        //   x = 0
        //   y = E
        //   n = 0
        //  nn = E0
        // nnn = 0E0

        // OUR TARGET: COSMAC VIP CHIP-8
        switch (instruction.f) {

            case 0x0:
                if (instruction.nn == 0xE0) {
                    instruction.opcode = opcode::Opcode::CLS; break;
                } else if (instruction.nn == 0xEE) {
                    instruction.opcode = opcode::Opcode::RET; break;
                } else {
                    instruction.opcode = opcode::Opcode::SYS_ADDR; break;
                }

            case 0x1:
                instruction.opcode = opcode::Opcode::JP_ADDR; break;

            case 0x2:
                instruction.opcode = opcode::Opcode::CALL_ADDR; break;

            case 0x3:
                instruction.opcode = opcode::Opcode::SE_V_B; break;
            
            case 0x4:
                instruction.opcode = opcode::Opcode::SNE_V_B; break;
            
            case 0x5:
                if (instruction.n != 0x0) break; // 5xy0 ONLY
                instruction.opcode = opcode::Opcode::SE_V_V; break;

            case 0x6:
                instruction.opcode = opcode::Opcode::LD_V_B; break;
            
            case 0x7:
                instruction.opcode = opcode::Opcode::ADD_V_B; break;

            case 0x8:
                switch (instruction.n) {
                    case 0x0: instruction.opcode = opcode::Opcode::LD_V_V; break;
                    case 0x1: instruction.opcode = opcode::Opcode::OR_V_V; break;
                    case 0x2: instruction.opcode = opcode::Opcode::AND_V_V; break;
                    case 0x3: instruction.opcode = opcode::Opcode::XOR_V_V; break;
                    case 0x4: instruction.opcode = opcode::Opcode::ADD_V_V; break;
                    case 0x5: instruction.opcode = opcode::Opcode::SUB_V_V; break;
                    case 0x6: instruction.opcode = opcode::Opcode::SHR_V; break;
                    case 0x7: instruction.opcode = opcode::Opcode::SUBN_V_V; break;
                    case 0xE: instruction.opcode = opcode::Opcode::SHL_V; break;
                }
                break;

            case 0x9:
                if (instruction.n != 0x0) break; // 9xy0 ONLY
                instruction.opcode = opcode::Opcode::SNE_V_V; break;
            
            case 0xA:
                instruction.opcode = opcode::Opcode::LD_I_ADDR; break;

            case 0xB:
                instruction.opcode = opcode::Opcode::JP_V_ADDR; break;

            case 0xC:
                instruction.opcode = opcode::Opcode::RND_V_B; break;

            case 0xD:
                instruction.opcode = opcode::Opcode::DRW_V_V; break;

            case 0xE:
                if (instruction.nn == 0x9E) {
                    instruction.opcode = opcode::Opcode::SKP_V; break;
                } else if (instruction.nn == 0xA1) {
                    instruction.opcode = opcode::Opcode::SKNP_V; break;
                } 
                break;

            case 0xF:
                switch (instruction.nn) {
                    case 0x07: instruction.opcode = opcode::Opcode::LD_V_DT; break;
                    case 0x0A: instruction.opcode = opcode::Opcode::LD_V_K; break;
                    case 0x15: instruction.opcode = opcode::Opcode::LD_DT_V; break;
                    case 0x18: instruction.opcode = opcode::Opcode::LD_ST_V; break;
                    case 0x1E: instruction.opcode = opcode::Opcode::ADD_I_V; break;
                    case 0x29: instruction.opcode = opcode::Opcode::LD_F_V; break;
                    case 0x33: instruction.opcode = opcode::Opcode::LD_B_V; break;
                    case 0x55: instruction.opcode = opcode::Opcode::LD_I_V; break;
                    case 0x65: instruction.opcode = opcode::Opcode::LD_V_I; break;
                    break;
                }
                break;
        }

        return instruction;
    }

}