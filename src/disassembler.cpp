#include "disassembler.h"
#include "opcode.h"
#include "chip8_specs.h"
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <optional>
#include <stdint.h>


namespace disassembler {
    // converts <uint8_t rom_bytes> to vector<string> ; returns std::nullopt if malformed ROM
    std::optional<std::vector<std::string>> disassemble(const std::vector<uint8_t>& rom_bytes) {
        std::vector<std::string> result;
        
        for (size_t i = 0; i < rom_bytes.size(); i+=2) {
            if (i+1 >= rom_bytes.size()) {
                std::cerr << "[disassembler]: Malformed ROM Detected!" << std::endl;
                return std::nullopt;
            }

            uint8_t byte_hi = rom_bytes[i];
            uint8_t byte_lo = rom_bytes[i+1];
            uint16_t combined = (static_cast<uint16_t>(byte_hi) << 8) | static_cast<uint16_t>(byte_lo); 
            
            opcode::Instruction decoded = opcode::decode(combined);
            uint16_t addr = START_ADDRESS_OFFSET + i;

            std::optional<std::string> maybe_mnemonic = mnemonic(decoded);
            std::string mnemonic_text = maybe_mnemonic.value_or("unknown"); // default to unknown if we dont support opcode

            std::string formatted = "opcode: " + to_hex_string(combined, 4) + " address: " 
                + to_hex_string(addr, 4) + " command: " + mnemonic_text;

            result.push_back(formatted);
        }
        
        return result;
    }

    std::optional<std::string> mnemonic(const opcode::Instruction& instruction) {

        // ex: [0x00E0]
        //   f = 0  :   dictates call type
        //   x = 0
        //   e = E
        //   n = 0
        //  nn = E0
        // nnn = 0E0
        switch (instruction.f) {

            // could be 00E0, 00EE, or 0nnn
            case (0x0):
                // clear screen
                if (instruction.nn == 0xE0) return "CLS";

                // return from subrutine to address on stack
                else if (instruction.nn == 0xEE) return "RET";

                // jump to native assembler subroutine at 0xNNN
                else {
                    return "SYS " + to_hex_string(instruction.nnn, 3);
                }

            case (0x1):
                // jump to address NNN
                return "JP " + to_hex_string(instruction.nnn, 3);

            case (0x2):
                // push return address onto stack and call subroutine at address NNN
                return "CALL " + to_hex_string(instruction.nnn, 3);

            case (0x3):
                // skip-if-equal : vX == NN
                return "SE v" + to_register_string(instruction.x) + ", " + to_hex_string(instruction.nn, 2);
            
            case (0x4):
                // skip-if-not-equal : vX != NN
                return "SNE v" + to_register_string(instruction.x) + ", " + to_hex_string(instruction.nn, 2);
            
            case (0x5):
                // skip-if-equal : vX == vY
                return "SE v" + to_register_string(instruction.x) + ", v" + to_register_string(instruction.y);

            case (0x6):
                // set vX to NN
                return "SET v" + to_register_string(instruction.x) + ", " + to_hex_string(instruction.nn, 2);
            
            case (0x7):
                // 	add vX to NN
                return "ADD v" + to_register_string(instruction.x) + to_hex_string(instruction.nn, 2);

            case (0x8):
                // set vX to the value of vY
                return "SET v" + to_register_string(instruction.x) + ", v" + to_hex_string(instruction.y, 2);

            default:
                return std::nullopt;
        }
    }

    std::string instruction_to_string(const opcode::Instruction& instruction) {
        std::ostringstream oss;
        oss << "f=" << std::setw(3) << static_cast<int>(instruction.f) << " "
            << "x=" << std::setw(3) << static_cast<int>(instruction.x) << " "
            << "y=" << std::setw(3) << static_cast<int>(instruction.y) << " "
            << "n=" << std::setw(3) << static_cast<int>(instruction.n) << " "
            << "nn=" << std::setw(5) << static_cast<int>(instruction.nn) << " "
            << "nnn=" << std::setw(5) << static_cast<int>(instruction.nnn);
        return oss.str();
    }

    std::string to_register_string(uint8_t val) {
        std::ostringstream oss;
        oss << std::hex << static_cast<int>(val);
        std::string formatted = oss.str();

        return formatted;
    }

    std::string to_hex_string(uint16_t value, int width) {
        std::ostringstream oss;

        oss << std::hex << std::setw(width) << std::setfill('0') << value;
        std::string formatted = oss.str();

        return formatted;
    }
}