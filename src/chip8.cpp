#include "chip8.h"
#include "rom_loader.h"
#include <fstream>
#include <iostream>
#include <algorithm>

chip8::chip8(const std::vector<uint8_t>& byte_stream) {
    if (byte_stream.size() + START_ADDRESS_OFFSET >= MEMORY_SIZE) {
        std::cerr << "[chip8] ERROR: Rom is too large for buffer!" << std::endl;
        throw std::invalid_argument("Rom is too large for buffer!");
    }

    std::copy(byte_stream.begin(), byte_stream.end(), memory.begin() + START_ADDRESS_OFFSET);
    std::cout << "[chip8] INFO: Successfully loaded " << byte_stream.size() << " bytes into memory." << std::endl;
}
