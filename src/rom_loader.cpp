#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <optional>
#include "rom_loader.h"

namespace rom_loader {

    std::optional<std::vector<uint8_t>> read_rom_bytes(const std::string& rom_path) {
        std::ifstream rom(rom_path, std::ios::binary | std::ios::ate);

        // check for extension
        size_t idx = rom_path.rfind('.');
        std::string fileExt = rom_path.substr(idx);
        if (fileExt != ".ch8") {
            std::cerr << "[rom_loader] ERROR: " << fileExt << " is not a valid chip8 ROM!" << std::endl;
            return std::nullopt;
        }


        // check if ROM can open successfully 
        if (!rom.is_open()) {
            std::cerr << "[rom_loader] ERROR: Failed to open: " << rom_path << std::endl;
            return std::nullopt;
        }

        // determine rom size
        const std::streamsize rom_size = rom.tellg();

        if (rom_size <= 0) return std::nullopt;

        // rewind pointer to begining for read
        rom.seekg(0, std::ios::beg);

        std::vector<uint8_t> buffer(rom_size);

        // failed to read rom into buffer
        if (!rom.read(reinterpret_cast<char*>(buffer.data()), rom_size)) {
            std::cerr << "[rom_loader] ERROR: Failed to read ROM into buffer!" << std::endl;
            return std::nullopt;
        }
        
        std::cout << "[rom_loader] INFO: loaded ROM of size " << rom_size << " bytes." << std::endl;
        return buffer;
    }
}