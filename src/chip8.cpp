#include "chip8.h"
#include <fstream>
#include <iostream>

// loads the rom, returns true if successful
bool chip8::load_rom(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        std::cerr << "Failed to open: " << path << std::endl;
        return false;
    }

    const std::streamsize ROM_SIZE = file.tellg();

    if (ROM_SIZE > (MEMORY_SIZE - START_ADDRESS)) {
        std::cerr << "ROM is too large for CHIP-8 memory!" << std::endl;
        return false;
    }

    file.seekg(0, std::ios::beg); // rewind pointer

    // read data into our fixed memory
    if (file.read(reinterpret_cast<char*>(memory.data() + START_ADDRESS), ROM_SIZE)) {
        std::cout << "Successfully read " << ROM_SIZE << " bytes." << std::endl;
        return true;
    }

    return false;
}