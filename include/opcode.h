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

    enum class Opcode {
        CLS,            // [00E0] clear the screen
        RET,            // [00EE] returns from subroutine to address pulled from stack
        SYS_ADDR,       // [0nnn] jump to native assembler rubroutine at 0xNNN
        JP_ADDR,        // [1nnn] jump to address NNN
        CALL_ADDR,      // [2nnn] push return address onto stack and call subroutine at address NNN
        SE_V_B,         // [3xnn] skip next opcode if vX == NN
        SNE_V_B,        // [4xnn] skip next opcode if vX != NN
        SE_V_V,         // [5xy0] skip next opcode if vX == vY
        LD_V_B,         // [6xnn] set vX to NN
        ADD_V_B,        // [7xnn] add NN to vX
        LD_V_V,         // [8xy0] set vX to the value of vY
        OR_V_V,         // [8xy1] set vX to (vX OR vY) bitwise
        AND_V_V,        // [8xy2] set vX to (vX AND vY) bitwise
        XOR_V_V,        // [8xy3] set vX to (vX XOR vY) bitwise
        ADD_V_V,        // [8xy4] add vY to vX, VF = 1 on overflow else 0
        SUB_V_V,        // [8xy5] subtract vY from vX, VF = 0 on underflow else 1
        SHR_V,          // [8xy6] set vX = vY, shift vX right 1 bit, VF = bit shifted out
        SUBN_V_V,       // [8xy7] set vX to (vY - vX), VF = 0 on underflow else 1
        SHL_V,          // [8xyE] set vX = vY, shift vX left 1 bit, VF = bit shifted out
        SNE_V_V,        // [9xy0] skip next opcode if vX != vY
        LD_I_ADDR,      // [Annn] set I to NNN
        JP_V_ADDR,      // [Bnnn] jump to address NNN + v0
        RND_V_B,        // [Cxnn] set vX to (random byte AND NN)
        DRW_V_V,        // [Dxyn] draw 8xN sprite at (vX, vY) from memory at I; VF = collision flag
        SKP_V,          // [Ex9E] skip next opcode if key in vX is pressed
        SKNP_V,         // [ExA1] skip next opcode if key in vX is NOT pressed
        LD_V_DT,        // [Fx07] set vX to the delay timer's value
        LD_V_K,         // [Fx0A] block until a key is pressed and released, store it in vX
        LD_DT_V,        // [Fx15] set the delay timer to vX
        LD_ST_V,        // [Fx18] set the sound timer to vX
        ADD_I_V,        // [Fx1E] add vX to I
        LD_F_V,         // [Fx29] set I to the address of the built-in hex sprite for vX's low nibble
        LD_B_V,         // [Fx33] write vX as BCD digits to memory at I, I+1, I+2
        LD_I_V,         // [Fx55] write registers v0..vX to memory starting at I 
        LD_V_I,         // [Fx65] read memory starting at I into registers v0..vX
        INVALID         // opcode didn't match any known pattern
    };

    Instruction decode(uint16_t raw_opcode);
}