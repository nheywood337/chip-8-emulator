#include <iostream>
#include <string>
#include "chip8.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Invalid execution, use ./chip8 <PATH_TO_ROM>" << std::endl;
        return -1;
    }

    const std::string ROM_TO_LOAD = argv[1];
    chip8 chip;

    // ROM load failure
    if (!chip.load_rom(ROM_TO_LOAD)) {
        return -1;
    }

    return 0;
}