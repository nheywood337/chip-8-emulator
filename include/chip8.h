#pragma once

#include "chip8_specs.h"
#include <cstdint>
#include <array>
#include <vector>
#include <stdexcept>
#include <string>
#include "opcode.h"

class chip8_error : public std::runtime_error {
    public:
        explicit chip8_error(const std::string& msg) : std::runtime_error(msg) {}
};

class chip8 {
    public:
        explicit chip8(const std::vector<uint8_t>& byte_stream);
        uint16_t fetch();                                           // FETCH STEP
        void execute(const opcode::Instruction& instruction);       // execution dispatcher (DECODE STEP)

        // getters for tests
        uint16_t get_program_counter() const;
        uint8_t get_register(uint8_t index) const;

    private:
        std::array<uint8_t, MEMORY_SIZE> memory = {};       // fixed memory size 4096
        uint16_t program_counter = START_ADDRESS_OFFSET;    // PC
        std::array<uint8_t, 16> v_registers = {};           // Vx

        // ----- execution functions -----

        // [6xnn] set vX to NN
        void execute_ld_v_b(const opcode::Instruction& instruction);
};