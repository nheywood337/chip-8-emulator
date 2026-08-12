#include "chip8.h"
#include "rom_loader.h"
#include <fstream>
#include <iostream>
#include <algorithm>

chip8::chip8(const std::string& path) {
    rom_path = path;
}

bool chip8::load_rom_into_memory() {
    auto result = rom_loader::read_rom_bytes(rom_path);

    // guard clause for read
    if (!result) {
        return false;
    }

    if (result->size() + START_ADDRESS_OFFSET <= MEMORY_SIZE) {
        std::copy(result->begin(), result->end(), memory.begin() + START_ADDRESS_OFFSET);
        std::cout << "[chip8] INFO: Successfully loaded " << result->size() << " bytes into memory." << std::endl;
    }
    else {
        std::cerr << "[chip8] ERROR: Rom is too large for buffer!" << std::endl;
        return false;
    }
    return true;
}
