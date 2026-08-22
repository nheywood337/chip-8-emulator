#pragma once

#include "chip8_specs.h"
#include <cstdint>
#include <array>
#include <vector>
#include <stdexcept>
#include <string>
#include "opcode.h"
#include <random>

class chip8_error : public std::runtime_error {
    public:
        explicit chip8_error(const std::string& msg) : std::runtime_error(msg) {}
};

class chip8 {
    public:
        explicit chip8(const std::vector<uint8_t>& byte_stream);
        uint16_t fetch();                                           // FETCH STEP
        void execute(const opcode::Instruction& instruction);       // execution dispatcher (DECODE STEP)

        void run(); // main loop for running the emulator

        // getters for tests
        uint16_t get_program_counter() const;
        uint8_t get_register(uint8_t index) const;
        uint16_t get_I() const;
        const std::array<uint8_t, DISPLAY_WIDTH * DISPLAY_HEIGHT>& get_display() const;
        const std::array<uint8_t, 16>& get_keypad() const;
        uint8_t get_delay_timer() const;
        uint8_t get_sound_timer() const;
        void set_keypad_state(uint8_t key, bool pressed);
        void set_delay_timer(uint8_t value);
        void set_sound_timer(uint8_t value);

    private:
        std::array<uint8_t, MEMORY_SIZE> memory = {};       // fixed memory size 4096
        uint16_t program_counter = START_ADDRESS_OFFSET;    // PC
        std::array<uint8_t, 16> v_registers = {};           // Vx
        std::array<uint16_t, 16> stack = {};                // stack
        uint8_t stack_pointer = 0;                          // pointer to next free slot on stack
        uint16_t I = 0;                                     // I

        uint8_t delay_timer = 0;                            // delay timer
        uint8_t sound_timer = 0;                            // sound timer
        std::array<uint8_t, 16> keypad = {};                // keypad state
        std::array<uint8_t, DISPLAY_WIDTH * DISPLAY_HEIGHT> display = {}; // display buffer
        
        // for random number generation
        std::mt19937 rng{std::random_device{}()}; 
        std::uniform_int_distribution<uint16_t> dist{0, 255};
        

        // ----- execution functions -----
        // [00E0] clear the screen
        void execute_cls();

        // [00EE] returns from subroutine to address pulled from stack
        void execute_ret();

        // [1nnn] jump to address NNN
        void execute_jp_addr(const opcode::Instruction& instruction);

        // [2nnn] push return address onto stack and call subroutine at address NNN
        void execute_call_addr(const opcode::Instruction& instruction);

        // [3xnn] skip next opcode if vX == NN
        void execute_se_v_b(const opcode::Instruction& instruction);

        // [4xnn] skip next opcode if vX != NN
        void execute_sne_v_b(const opcode::Instruction& instruction);

        // [5xy0] skip next opcode if vX == vY
        void execute_se_v_v(const opcode::Instruction& instruction);

        // [6xnn] set vX to NN
        void execute_ld_v_b(const opcode::Instruction& instruction);

        // [7xnn] add NN to vX
        void execute_add_v_b(const opcode::Instruction& instruction);

        // [8xy0] set vX to the value of vY
        void execute_ld_v_v(const opcode::Instruction& instruction);

        // [8xy1] set vX to (vX OR vY) bitwise
        void execute_or_v_v(const opcode::Instruction& instruction);

        // [8xy2] set vX to (vX AND vY) bitwise
        void execute_and_v_v(const opcode::Instruction& instruction);

        // [8xy3] set vX to (vX XOR vY) bitwise
        void execute_xor_v_v(const opcode::Instruction& instruction);

        // [8xy4] add vY to vX, VF = 1 on overflow else 0
        void execute_add_v_v(const opcode::Instruction& instruction);

        // [8xy5] subtract vY from vX, VF = 0 on underflow else 1
        void execute_sub_v_v(const opcode::Instruction& instruction);

        // [8xy6] set vX = vY, shift vX right 1 bit, VF = bit shifted out
        void execute_shr_v(const opcode::Instruction& instruction);

        // [8xy7] set vX to (vY - vX), VF = 0 on underflow else 1
        void execute_subn_v_v(const opcode::Instruction& instruction);

        // [8xyE] set vX = vY, shift vX left 1 bit, VF = bit shifted out
        void execute_shl_v(const opcode::Instruction& instruction);

        // [9xy0] skip next opcode if vX != vY
        void execute_sne_v_v(const opcode::Instruction& instruction);

        // [Annn] set I to NNN
        void execute_ld_i_addr(const opcode::Instruction& instruction);

        // [Bnnn] jump to address NNN + v0
        void execute_jp_v_addr(const opcode::Instruction& instruction);

        // [Cxnn] set vX to (random byte AND NN)
        void execute_rnd_v_b(const opcode::Instruction& instruction);

        // [Dxyn] draw sprite at (vX, vY) with height N, VF = collision
        void execute_drw_v_v(const opcode::Instruction& instruction);

        // [Ex9E] skip next opcode if key in vX is pressed
        void execute_skp_v(const opcode::Instruction& instruction);

        // [ExA1] skip next opcode if key in vX is NOT pressed
        void execute_sknp_v(const opcode::Instruction& instruction);

        // [Fx15] Set delay timer to vX. VF is not affected.
        void execute_ld_dt_v(const opcode::Instruction& instruction);

        // [Fx18] Set sound timer to vX. VF is not affected.
        void execute_ld_st_v(const opcode::Instruction& instruction);

        // [Fx1E] add vX to I
        void execute_add_i_v(const opcode::Instruction& instruction);

};