#include "chip8.h"
#include <fstream>
#include <iostream>
#include <algorithm>

chip8::chip8(const std::vector<uint8_t>& byte_stream) {
    // Guard for a ROM that's too large for our buffer
    if (byte_stream.size() + START_ADDRESS_OFFSET > MEMORY_SIZE) {
        throw chip8_error("[chip8] ERROR: Rom is too large for buffer!");
    }

    std::copy(byte_stream.begin(), byte_stream.end(), memory.begin() + START_ADDRESS_OFFSET);
    std::cout << "[chip8] INFO: Successfully loaded " << byte_stream.size() << " bytes into memory." << std::endl;
}

uint16_t chip8::fetch() {
    try {
        uint8_t hi = this->memory.at(this->program_counter);
        uint8_t lo = this->memory.at(this->program_counter + 1);

        uint16_t combined = (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo); 

        this->program_counter += 2;
        
        return combined;
    }
    catch (const std::out_of_range& e) {
        throw chip8_error(std::string("[chip8]: read out of range: ") + e.what());
    }
}
