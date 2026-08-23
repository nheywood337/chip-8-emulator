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
    constexpr int WINDOW_SCALE = 12;                   // 64x32 -> 768x384
    constexpr int DEFAULT_INSTRUCTIONS_PER_FRAME = 11; // ~660 Hz CPU rate

    // 60 Hz = ~16.67ms per frame
    constexpr std::chrono::nanoseconds FRAME_DURATION(1000000000 / 60);

    struct options {
        int instructions_per_frame = DEFAULT_INSTRUCTIONS_PER_FRAME;
        long frame_limit = 0; // 0 = run until the user quits
    };

    void print_usage() {
        std::cerr << "usage: ./chip8 -r <PATH_TO_ROM> [--ipf N] [--frames N]\n"
                  << "       ./chip8 -d <PATH_TO_ROM>\n"
                  << "  --ipf N     instructions per frame, default "
                  << DEFAULT_INSTRUCTIONS_PER_FRAME << " (some ROMs are timing sensitive)\n"
                  << "  --frames N  quit after N frames, for smoke tests\n";
    }

    bool parse_options(int argc, char* argv[], options& opts) {
        for (int i = 3; i < argc; ++i) {
            const std::string flag = argv[i];

            if (i + 1 >= argc) {
                std::cerr << "ERROR: " << flag << " needs a value" << std::endl;
                return false;
            }

            const std::string value = argv[++i];

            try {
                if (flag == "--ipf") {
                    opts.instructions_per_frame = std::stoi(value);
                    if (opts.instructions_per_frame < 1) throw std::out_of_range("must be >= 1");
                }
                else if (flag == "--frames") {
                    opts.frame_limit = std::stol(value);
                    if (opts.frame_limit < 1) throw std::out_of_range("must be >= 1");
                }
                else {
                    std::cerr << "ERROR: unknown option " << flag << std::endl;
                    return false;
                }
            }
            catch (const std::exception&) {
                std::cerr << "ERROR: bad value for " << flag << ": " << value << std::endl;
                return false;
            }
        }

        return true;
    }

    // one iteration = one 60 Hz frame
    void run_emulator(chip8& chip, platform& plat, const options& opts) {
        using clock = std::chrono::steady_clock;
        auto next_frame = clock::now();

        for (long frame = 0; plat.poll_events(); ++frame) {
            if (opts.frame_limit > 0 && frame >= opts.frame_limit) {
                break;
            }

            next_frame += FRAME_DURATION;

            // the keypad lives in both places; 16 bytes a frame is the price of
            // chip8_core not knowing SDL exists
            const auto& keys = plat.get_keypad();
            for (uint8_t k = 0; k < 16; ++k) {
                chip.set_keypad_state(k, keys[k] != 0);
            }

            for (int i = 0; i < opts.instructions_per_frame; ++i) {
                chip.step();
            }

            chip.tick_timers();
            plat.render(chip.get_display());

            std::this_thread::sleep_until(next_frame);

            // if we fell behind, resync rather than sprint through the backlog
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
    if (argc < 3) {
        print_usage();
        return EXIT_FAILURE;
    }

    const MODE mode = initialize_mode(argv[1]);

    if (mode == MODE::FAILURE) {
        std::cerr << "ERROR: Invalid flag " << argv[1] << std::endl;
        print_usage();
        return EXIT_FAILURE;
    }

    options opts;
    if (!parse_options(argc, argv, opts)) {
        print_usage();
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
            run_emulator(chip, plat, opts);
        }
        catch (const platform_error& e) {
            std::cerr << e.what() << std::endl;
            return EXIT_FAILURE;
        }
        catch (const chip8_error& e) {
            std::cerr << e.what() << std::endl;
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
