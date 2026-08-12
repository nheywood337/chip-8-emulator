#include "chip8.h"
#include <fstream>
#include <iostream>

// loads the rom, returns true if successful
bool chip8::load_rom(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);

    // check if file can open successfully 
    if (!file.is_open()) {
        std::cerr << "Failed to open: " << path << std::endl;
        return false;
    }

    // determine rom size
    const std::streamsize ROM_SIZE = file.tellg();

    // determine if ROM size is too large for our buffer
    if (ROM_SIZE > (MEMORY_SIZE - START_ADDRESS)) {
        std::cerr << "ROM is too large for CHIP-8 memory!" << std::endl;
        return false;
    }

    // rewind pointer to begining for read
    file.seekg(0, std::ios::beg);

    // read data into our fixed memory buffer
    if (file.read(reinterpret_cast<char*>(memory.data() + START_ADDRESS), ROM_SIZE)) {
        std::cout << "Successfully read " << ROM_SIZE << " bytes." << std::endl;
        return true;
    }

    return false;
}