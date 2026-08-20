#include "chip8.h"
#include "disassembler.h"
#include <fstream>
#include <iostream>
#include <algorithm>

chip8::chip8(const std::vector<uint8_t>& byte_stream) {
    // Guard for a ROM that's too large for our buffer
    if (byte_stream.size() + START_ADDRESS_OFFSET > MEMORY_SIZE) {
        throw chip8_error("[chip8] ERROR: Rom is too large for buffer!");
    }

    std::copy(byte_stream.begin(), byte_stream.end(), memory.begin() + START_ADDRESS_OFFSET);
    std::cout << "[chip8] INFO: Successfully loaded " << byte_stream.size() << " bytes into memory." << std::endl;
}

uint16_t chip8::fetch() {
    try {
        uint8_t hi = this->memory.at(this->program_counter);
        uint8_t lo = this->memory.at(this->program_counter + 1);

        uint16_t combined = (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo); 

        this->program_counter += 2;
        
        return combined;
    }
    catch (const std::out_of_range& e) {
        throw chip8_error(std::string("[chip8]: read out of range: ") + e.what());
    }
}

// Main execution dispatcher
void chip8::execute(const opcode::Instruction& instruction) {
    switch (instruction.opcode) {
        case opcode::Opcode::JP_ADDR: execute_jp_addr(instruction); break;
        case opcode::Opcode::LD_V_B: execute_ld_v_b(instruction); break;
        case opcode::Opcode::ADD_V_B: execute_add_v_b(instruction); break;

        default: throw chip8_error("[chip8]: opcode invalid or not supported yet: " + disassembler::instruction_to_string(instruction));
    }
}

// ----- Executors -----

// [1nnn] jump to address NNN
void chip8::execute_jp_addr(const opcode::Instruction& instruction) {
    this->program_counter = instruction.nnn;
}

// [6xnn] set vX to NN
void chip8::execute_ld_v_b(const opcode::Instruction& instruction) {
    this->v_registers[instruction.x] = instruction.nn;
}

// [7xnn] add NN to vX
void chip8::execute_add_v_b(const opcode::Instruction& instruction) {
    this->v_registers[instruction.x] += instruction.nn;
}

// ----- Helpers for tests -----
uint16_t chip8::get_program_counter() const {
    return this->program_counter;
}

uint8_t chip8::get_register(uint8_t index) const {
    try {
        return this->v_registers.at(index);
    }
    catch (const std::out_of_range& e) {
        throw chip8_error(std::string("[chip8]: register out of bounds: ") + e.what());
    }
}
