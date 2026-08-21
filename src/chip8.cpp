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
        case opcode::Opcode::RET:       execute_ret();                  break;
        case opcode::Opcode::JP_ADDR:   execute_jp_addr(instruction);   break;
        case opcode::Opcode::CALL_ADDR: execute_call_addr(instruction); break;
        case opcode::Opcode::SE_V_B:    execute_se_v_b(instruction);    break;
        case opcode::Opcode::SNE_V_B:   execute_sne_v_b(instruction);   break;
        case opcode::Opcode::SE_V_V:    execute_se_v_v(instruction);    break;
        case opcode::Opcode::LD_V_B:    execute_ld_v_b(instruction);    break;
        case opcode::Opcode::ADD_V_B:   execute_add_v_b(instruction);   break;
        case opcode::Opcode::LD_V_V:    execute_ld_v_v(instruction);    break;
        case opcode::Opcode::OR_V_V:    execute_or_v_v(instruction);    break;
        case opcode::Opcode::AND_V_V:   execute_and_v_v(instruction);   break;
        case opcode::Opcode::XOR_V_V:   execute_xor_v_v(instruction);   break;
        case opcode::Opcode::ADD_V_V:   execute_add_v_v(instruction);   break;
        case opcode::Opcode::SUB_V_V:   execute_sub_v_v(instruction);   break;
        case opcode::Opcode::SHR_V:     execute_shr_v(instruction);     break;
        case opcode::Opcode::SUBN_V_V:  execute_subn_v_v(instruction);  break;
        case opcode::Opcode::SHL_V:     execute_shl_v(instruction);     break;
        case opcode::Opcode::SNE_V_V:   execute_sne_v_v(instruction);   break;

        default: throw chip8_error("[chip8]: opcode invalid or not supported yet: " + disassembler::instruction_to_string(instruction));
    }
}

// ----- Helpers -----
void chip8::advance_pc() {
    program_counter += 2;
}

// ----- Executors -----

// [00EE] returns from subroutine to address pulled from stack
void chip8::execute_ret() {
    if (this->stack_pointer == 0) throw chip8_error("[chip8]: execute_ret - stack underflow!");

    this->stack_pointer--; 
    this->program_counter = this->stack.at(this->stack_pointer);
}

// [1nnn] jump to address NNN
void chip8::execute_jp_addr(const opcode::Instruction& instruction) {
    this->program_counter = instruction.nnn;
}

// [2nnn] push return address onto stack and call subroutine at address NNN
void chip8::execute_call_addr(const opcode::Instruction& instruction) {
    if (this->stack_pointer >= 16) throw chip8_error("[chip8]: execute_call_addr - stack pointer out of bounds!");

    this->stack.at(this->stack_pointer) = this->program_counter; // Save return address
    this->stack_pointer++;
    this->program_counter = instruction.nnn;       // Jump to NNN
}

// [3xnn] skip next opcode if vX == NN
void chip8::execute_se_v_b(const opcode::Instruction& instruction) {
    if (this->get_register(instruction.x) == instruction.nn) {
        advance_pc();
    }
}

// [4xnn] skip next opcode if vX != NN
void chip8::execute_sne_v_b(const opcode::Instruction& instruction) {
    if (this->get_register(instruction.x) != instruction.nn) {
        advance_pc();
    }
}

// [5xy0] skip next opcode if vX == vY
void chip8::execute_se_v_v(const opcode::Instruction& instruction) {
    if (this->get_register(instruction.x) == get_register(instruction.y)) {
        advance_pc();
    }
}

// [6xnn] set vX to NN
void chip8::execute_ld_v_b(const opcode::Instruction& instruction) {
    this->v_registers.at(instruction.x) = instruction.nn;
}

// [7xnn] add NN to vX
void chip8::execute_add_v_b(const opcode::Instruction& instruction) {
    this->v_registers.at(instruction.x) += instruction.nn;
}

// [8xy0] set vX to the value of vY
void chip8::execute_ld_v_v(const opcode::Instruction& instruction) {
    this->v_registers.at(instruction.x) = this->v_registers.at(instruction.y);
}

// [8xy1] set vX to (vX OR vY) bitwise
void chip8::execute_or_v_v(const opcode::Instruction& instruction) {
    this->v_registers.at(instruction.x) |= this->v_registers.at(instruction.y);
    this->v_registers.at(VF) = 0; // COSMAC VIP side effect quirk
}

// [8xy2] set vX to (vX AND vY) bitwise
void chip8::execute_and_v_v(const opcode::Instruction& instruction) {
    this->v_registers.at(instruction.x) &= this->v_registers.at(instruction.y);
    this->v_registers.at(VF) = 0; // COSMAC VIP side effect quirk
}

// [8xy3] set vX to (vX XOR vY) bitwise
void chip8::execute_xor_v_v(const opcode::Instruction& instruction) {
    this->v_registers.at(instruction.x) ^= this->v_registers.at(instruction.y);
    this->v_registers.at(VF) = 0; // COSMAC VIP side effect quirk
}

// [8xy4] add vY to vX, VF = 1 on overflow else 0
void chip8::execute_add_v_v(const opcode::Instruction& instruction) {
    uint8_t vx = this->v_registers.at(instruction.x);
    uint8_t vy = this->v_registers.at(instruction.y);
    int sum = vx + vy;
    bool overflowed = sum > 255;
    uint8_t truncated = static_cast<uint8_t>(sum);

    this->v_registers.at(instruction.x) = truncated;
    if (overflowed) {
        this->v_registers.at(VF) = 1;
    }
    else this->v_registers.at(VF) = 0;
}

// [8xy5] subtract vY from vX, VF = 0 on underflow else 1
void chip8::execute_sub_v_v(const opcode::Instruction& instruction) {
    uint8_t vx = this->v_registers.at(instruction.x);
    uint8_t vy = this->v_registers.at(instruction.y);
    int diff = vx - vy;
    bool underflowed = diff < 0;
    uint8_t truncated = static_cast<uint8_t>(diff);

    this->v_registers.at(instruction.x) = truncated;
    if (underflowed) {
        this->v_registers.at(VF) = 0;
    }
    else this->v_registers.at(VF) = 1;
}

// [8xy6] set vX = vY, shift vX right 1 bit, VF = bit shifted out
void chip8::execute_shr_v(const opcode::Instruction& instruction) {
    // Copy VY into VX (COSMAC VIP behavior)
    this->v_registers.at(instruction.x) = this->v_registers.at(instruction.y);
    
    uint8_t vx = this->v_registers.at(instruction.x);

    // Perform shift
    this->v_registers.at(instruction.x) >>= 1;

    this->v_registers.at(VF) = vx & 0x1;
}

// [8xy7] Set vX = vY - vX. VF = 1 if NO borrow (vY >= vX), else 0
void chip8::execute_subn_v_v(const opcode::Instruction& instruction) {
    uint8_t vx = this->v_registers.at(instruction.x);
    uint8_t vy = this->v_registers.at(instruction.y);

    this->v_registers.at(instruction.x) = vy - vx;

    // Set VF to 1 if vY >= vX (no underflow/borrow), else 0
    this->v_registers.at(VF) = (vy >= vx) ? 1 : 0;
}

// [8xyE] Set vX = vY, shift vX left 1 bit, VF = bit shifted out (MSB)
void chip8::execute_shl_v(const opcode::Instruction& instruction) {
    // Copy VY into VX (COSMAC VIP behavior)
    this->v_registers.at(instruction.x) = this->v_registers.at(instruction.y);

    uint8_t vx = this->v_registers.at(instruction.x);

    // Perform shift
    this->v_registers.at(instruction.x) <<= 1;

    this->v_registers.at(VF) = (vx & 0x80) >> 7;
}

// [9xy0] skip next opcode if vX != vY
void chip8::execute_sne_v_v(const opcode::Instruction& instruction) {
    if (this->get_register(instruction.x) != get_register(instruction.y)) {
        advance_pc();
    }
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
