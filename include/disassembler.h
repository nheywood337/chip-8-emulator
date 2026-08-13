#pragma once

#include "opcode.h"
#include <vector>
#include <string>
#include <stdint.h>
#include <optional>


namespace disassembler {
    std::optional<std::vector<std::string>> disassemble(const std::vector<uint8_t>& rom_bytes);
    std::string instruction_to_string(const opcode::Instruction& instruction);
    std::optional<std::string> mnemonic(const opcode::Instruction& instruction);
    std::string to_hex(uint16_t value, int width);
}