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
        result.push_back(" ADR |  OP  | INSTRUCTION");
        
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

            std::string formatted = to_hex_string(addr, 4) + " | " 
                + to_hex_string(combined, 4) + " | " + mnemonic_text;

            result.push_back(formatted);
        }
        
        return result;
    }

    std::optional<std::string> mnemonic(const opcode::Instruction& instruction) {

        // ex: [0x00E0]
        //   f = 0  :   dictates call type
        //   x = 0
        //   y = E
        //   n = 0
        //  nn = E0
        // nnn = 0E0

        // OUR TARGET: COSMAC VIP CHIP-8
        switch (instruction.f) {

            // could be 00E0, 00EE, or 0nnn
            case (0x0):
                // CLS
                if (instruction.nn == 0xE0) return "CLS";

                // RET
                else if (instruction.nn == 0xEE) return "RET";

                // SYS addr
                else {
                    return "SYS " + to_hex_string(instruction.nnn, 3);
                }

            case (0x1):
                // JP addr
                return "JP " + to_hex_string(instruction.nnn, 3);

            case (0x2):
                // CALL addr
                return "CALL " + to_hex_string(instruction.nnn, 3);

            case (0x3):
                // SE Vx, byte
                return "SE V" + to_register_string(instruction.x) + ", " 
                    + to_hex_string(instruction.nn, 2);
            
            case (0x4):
                // SNE Vx, byte
                return "SNE V" + to_register_string(instruction.x) 
                    + ", " + to_hex_string(instruction.nn, 2);
            
            case (0x5):
                // SE Vx, Vy
                return "SE V" + to_register_string(instruction.x) 
                    + ", V" + to_register_string(instruction.y);

            case (0x6):
                // LD Vx, byte
                return "LD V" + to_register_string(instruction.x) 
                    + ", " + to_hex_string(instruction.nn, 2);
            
            case (0x7):
                // 	ADD Vx, byte
                return "ADD V" + to_register_string(instruction.x) 
                    + ", " + to_hex_string(instruction.nn, 2);

            // could be 8xy0, 8xy1, 8xy2, 8xy3, 8xy4, 8xy5, 8xy6
            case (0x8):
                switch (instruction.n) {
                    case (0):
                        // LD Vx, Vy
                        return "LD V" + to_register_string(instruction.x) 
                            + ", V" + to_register_string(instruction.y);
                    
                    case (1):
                        // OR Vx, Vy <COSMAC based variants will reset VF>
                        return "OR V" + to_register_string(instruction.x) 
                            + ", V" + to_register_string(instruction.y);
                
                    case (2):
                        // AND Vx, Vy <COSMAC based variants will reset VF>
                        return "AND V" + to_register_string(instruction.x) 
                            + ", V" + to_register_string(instruction.y);
                
                    case (3):
                        // XOR Vx, Vy <COSMAC based variants will reset VF>
                        return "XOR V" + to_register_string(instruction.x) 
                            + ", V" + to_register_string(instruction.y);

                    case (4):
                        // ADD Vx, Vy
                        return "ADD V" + to_register_string(instruction.x) 
                            + ", V" + to_register_string(instruction.y);

                    case (5):
                        // SUB Vx, Vy
                        return "SUB V" + to_register_string(instruction.x) 
                            + ", V" + to_register_string(instruction.y);

                    case (6):
                        // SHR Vx {, Vy}
                        return "SHR V" + to_register_string(instruction.x) 
                            + ", V" + to_register_string(instruction.y);

                    case (7):
                        // SUBN Vx, Vy
                        return "SUBN V" + to_register_string(instruction.x) 
                            + ", V" + to_register_string(instruction.y);

                    case (0xE):
                        // SHL Vx {, Vy}
                        return "SHL V" + to_register_string(instruction.x) 
                            + ", V" + to_register_string(instruction.y);
                
                    default: return std::nullopt;
                }

            case (9):
                // SNE Vx, Vy
                return "SNE V" + to_register_string(instruction.x) 
                    + ", V" + to_register_string(instruction.y);
            
            case (0xA):
                // LD I, addr
                return "LD I, " + to_hex_string(instruction.nnn, 3);

            case (0xB):
                // JP V0, addr
                return "JP V0, " + to_hex_string(instruction.nnn, 3);

            case (0xC):
                // RND Vx, byte
                return "RND V" + to_register_string(instruction.x) 
                    + ", " + to_hex_string(instruction.nn, 2);

            case (0xD):
                // DRW Vx, Vy, nibble
                return "DRW V" + to_register_string(instruction.x) + ", V" 
                    + to_register_string(instruction.y) + ", " + to_hex_string(instruction.n, 1);

            case (0xE):
                if (instruction.n == 0xE) {
                    // SKP Vx
                    return "SKP V" + to_register_string(instruction.x);
                }
                else if (instruction.n == 0x1) {
                    // SKNP Vx
                    return "SKNP V" + to_register_string(instruction.x);
                }
                else {
                    return std::nullopt;
                }

            case (0xF):
                switch (instruction.nn) {
                    case (0x07):
                        // LD Vx, DT
                        return "LD V" + to_register_string(instruction.x) + ", DT";

                    case (0x0A):
                        // LD Vx, K
                        return "LD V" + to_register_string(instruction.x) + ", K";

                    case (0x15):
                        // LD DT, Vx
                        return "LD DT, V" + to_register_string(instruction.x);

                    case (0x18):
                        // LD ST, Vx
                        return "LD ST, V" + to_register_string(instruction.x);

                    case (0x1E):
                        // ADD I, Vx
                        return "ADD I, V" + to_register_string(instruction.x);

                    case (0x29):
                        // LD F, Vx
                        return "LD F, V" + to_register_string(instruction.x);

                    case (0x33):
                        // LD B, Vx
                        return "LD B, V" + to_register_string(instruction.x);

                    case (0x55):
                        // LD [I], Vx
                        return "LD [I], V" + to_register_string(instruction.x);

                    case (0x65):
                        // LD Vx, [I]
                        return "LD V" + to_register_string(instruction.x) + ", [I]";

                    default: return std::nullopt;
                }

            [[fallthrough]];
            default:
                return std::nullopt;
        }
    }

    // used for debugging
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

    // takes in uint8_t and returns hex friendly string
    std::string to_register_string(uint8_t val) {
        std::ostringstream oss;
        oss << std::hex << static_cast<int>(val);
        std::string formatted = oss.str();

        return formatted;
    }

    // takes in uint16_t, and width; returns hex friendly string
    std::string to_hex_string(uint16_t value, int width) {
        std::ostringstream oss;

        oss << std::hex << std::setw(width) << std::setfill('0') << value;
        std::string formatted = oss.str();

        return formatted;
    }
}