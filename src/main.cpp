#include <iostream>
#include <string>
#include "chip8.h"

const std::string ROM_DIRECTORY = "../roms";
const std::string ROM_TO_LOAD = ROM_DIRECTORY + "/1-chip8-logo.ch8";

int main() {
    chip8 chip;
    
    chip.load_rom(ROM_TO_LOAD);

    return 0;
}