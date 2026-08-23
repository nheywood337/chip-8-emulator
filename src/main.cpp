#include <iostream>
#include <cstdlib>
#include <chrono>
#include <thread>
#include "chip8.h"
#include "platform.h"
#include "rom_loader.h"
#include "disassembler.h"


enum class MODE {
    RUN,
    DISASSEMBLE,
    FAILURE
};

namespace {
    constexpr int WINDOW_SCALE = 12;           // 64x32 -> 768x384
    constexpr int INSTRUCTIONS_PER_FRAME = 11; // ~660 Hz CPU rate

    // 60 Hz = ~16.67ms per frame
    constexpr std::chrono::nanoseconds FRAME_DURATION(1000000000 / 60);

    // one iteration = one 60 Hz frame
    void run_emulator(chip8& chip, platform& plat) {
        using clock = std::chrono::steady_clock;
        auto next_frame = clock::now();

        while (plat.poll_events()) {
            next_frame += FRAME_DURATION;

            const auto& keys = plat.get_keypad();
            for (uint8_t k = 0; k < 16; k++) {
                chip.set_keypad_state(k, keys[k] != 0);
            }

            for (int i = 0; i < INSTRUCTIONS_PER_FRAME; i++) {
                chip.step();
            }

            chip.tick_timers();
            plat.render(chip.get_display());

            std::this_thread::sleep_until(next_frame);

            const auto now = clock::now();
            if (next_frame + FRAME_DURATION < now) {
                next_frame = now;
            }
        }
    }
}

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

    const MODE mode = initialize_mode(argv[1]);

    if (mode == MODE::FAILURE) {
        std::cerr << "ERROR: Invalid flag " << argv[1] << std::endl;
        return EXIT_FAILURE;
    }

    const std::string ROM_TO_LOAD = argv[2];
    auto result = rom_loader::read_rom_bytes(ROM_TO_LOAD);
    // guard clause for read
    if (!result) {
        return EXIT_FAILURE;
    }

    if (mode == MODE::RUN) {
        try {
            chip8 chip(*result);
            platform plat("CHIP-8", WINDOW_SCALE);
            run_emulator(chip, plat);
        }
        catch (const platform_error& e) {
            std::cerr << e.what() << std::endl;
            return EXIT_FAILURE;
        }
        catch (const chip8_error& e) {
            std::cerr << e.what() << std::endl;
            return EXIT_FAILURE;
        }
        catch (const std::exception& e) {
            std::cerr << "[main]: unexpected error: " << e.what() << std::endl;
            return EXIT_FAILURE;
        }
    }
    else if (mode == MODE::DISASSEMBLE) {

        auto output = disassembler::disassemble(*result);

        if (!output) {
            return EXIT_FAILURE;
        }

        std::cout << " ADR |  OP  | INSTRUCTION" << std::endl; // print header
        for (const auto& line : *output) {
            std::cout << line << std::endl;
        }
    }

    return 0;
}
