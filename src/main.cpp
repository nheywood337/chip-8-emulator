#include <iostream>
#include <string>
#include "chip8.h"

const std::string ROM_DIRECTORY = "../roms";
const std::string ROM_TO_LOAD = ROM_DIRECTORY + "/1-chip8-logo.ch8";

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Invalid execution, use ./chip8 <PATH_TO_ROM>" << std::endl;
        return -1;
    }

    const std::string ROM_TO_LOAD = argv[1];
    chip8 chip;

    chip.load_rom(ROM_TO_LOAD);

    return 0;
}