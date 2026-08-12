#pragma once

#include <vector>
#include <string>
#include <stdint.h>


namespace disassembler {
    std::vector<std::string> disassemble(const std::vector<uint8_t>& rom_bytes);

}