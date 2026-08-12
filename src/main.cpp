#include <iostream>
#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include "chip8.h"
#include "rom_loader.h"
#include "disassembler.h"

enum MODE {
    RUN,
    DISASSEMBLE,
    INIT,
    FAILURE
};

MODE initialize_mode(const std::string& mode) {
    if (mode == "-r") {
        return MODE::RUN;
    }
    else if (mode == "-d") {
        return MODE::DISASSEMBLE;
    }
    else {
        return MODE::FAILURE;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "ERROR: Invalid execution, ex: ./chip8 -r <PATH_TO_ROM>" << std::endl;
        return EXIT_FAILURE;
    }
    MODE mode = MODE::INIT;


    initialize_mode(argv[1]);
    const std::string ROM_TO_LOAD = argv[2];

    if (MODE::RUN) {
        chip8 chip(ROM_TO_LOAD);
        return 0;
    }
    else if (MODE::DISASSEMBLE) {
        auto result = rom_loader::read_rom_bytes(ROM_TO_LOAD);

        // guard clause for read
        if (!result) {
            return false;
        }
        
        disassembler::disassemble(result->data());
        return 0;
    }

    const std::string ROM_TO_LOAD = argv[3];
    chip8 chip(ROM_TO_LOAD);

    if (!chip.load_rom_into_memory()) {
        return EXIT_FAILURE;
    }

    return 0;
}