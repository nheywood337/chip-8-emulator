#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <array>

/*
    Initial prototype
    TODO: cleanup
*/

const std::string ROM_DIRECTORY = "../roms";
const std::string ROM_TO_LOAD = ROM_DIRECTORY + "/1-chip8-logo.ch8";
const uint16_t MEMORY_SIZE = 4096;
const uint16_t START_ADDRESS = 0x200;

int main() {


    std::array<uint8_t, MEMORY_SIZE> memory = {0}; // fixed memory size 4096
    std::ifstream file(ROM_TO_LOAD, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        std::cerr << "Failed to open: " << ROM_TO_LOAD << std::endl;
        return 1;
    }

    const std::streamsize ROM_SIZE = file.tellg();

    if (ROM_SIZE > (MEMORY_SIZE - START_ADDRESS)) {
        std::cerr << "ROM is too large for CHIP-8 memory!" << std::endl;
        return 1;
    }

    file.seekg(0, std::ios::beg); // rewind pointer

    // read data into our fixed memory
    if (file.read(reinterpret_cast<char*>(memory.data() + START_ADDRESS), ROM_SIZE)) {
        std::cout << "Successfully read " << ROM_SIZE << " bytes." << std::endl;
    }

    return 0;
}