#pragma once

#include <string>
#include <fstream>
#include <vector>
#include <cstdint>
#include <optional>


namespace rom_loader {
    std::optional<std::vector<uint8_t>> read_rom_bytes(const std::string& file_path);
}
