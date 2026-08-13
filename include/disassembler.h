#pragma once

#include <vector>
#include <string>
#include <stdint.h>
#include <optional>


namespace disassembler {
    std::optional<std::vector<std::string>> disassemble(const std::vector<uint8_t>& rom_bytes);

}