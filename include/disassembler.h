#pragma once

#include "opcode.h"
#include <vector>
#include <string>
#include <stdint.h>


namespace disassembler {
    std::vector<std::string> disassemble(const std::vector<uint8_t>& rom_bytes);
    std::string instruction_to_string(const opcode::Instruction& instruction);
    std::string mnemonic(const opcode::Instruction& instruction);
    std::string to_hex_string(uint16_t value, int width);
    std::string to_register_string(uint8_t val);
}