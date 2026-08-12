#include <iostream>
#include <string>
#include <cstdlib>
#include "chip8.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "ERROR: Invalid execution, use ./chip8 <PATH_TO_ROM>" << std::endl;
        return EXIT_FAILURE;
    }

    const std::string ROM_TO_LOAD = argv[1];
    chip8 chip(ROM_TO_LOAD);

    if (!chip.load_rom_into_memory()) {
        return EXIT_FAILURE;
    }

    return 0;
}