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
    if (argc != 3) {
        std::cerr << "ERROR: Invalid execution, ex: ./chip8 -r <PATH_TO_ROM>" << std::endl;
        return EXIT_FAILURE;
    }
    MODE mode = MODE::INIT;

    mode = initialize_mode(argv[1]);
    const std::string ROM_TO_LOAD = argv[2];

    if (mode == MODE::RUN) {
        chip8 chip(ROM_TO_LOAD);
        if (!chip.load_rom_into_memory()) {
            return EXIT_FAILURE;
        }
    }
    else if (mode == MODE::DISASSEMBLE) {
        auto result = rom_loader::read_rom_bytes(ROM_TO_LOAD);

        // guard clause for read
        if (!result) {
            return EXIT_FAILURE;
        }
        
        auto output = disassembler::disassemble(*result);

        for (const auto& line : output) {
            std::cout << line << std::endl;
        }
    }

    return 0;
}