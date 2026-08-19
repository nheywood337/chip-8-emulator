#include "disassembler.h"
#include "opcode.h"
#include "chip8_specs.h"
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdint.h>
#include <optional>


namespace disassembler {
    // converts <uint8_t rom_bytes> to vector<string> ; returns nullopt on invalid rom
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

            std::string mnemonic_text = mnemonic(decoded);

            std::string formatted = to_hex_string(addr, 4) + " | " 
                + to_hex_string(combined, 4) + " | " + mnemonic_text;

            result.push_back(formatted);
        }
        
        return result;
    }

    std::string mnemonic(const opcode::Instruction& instruction) {

        // ex: [0x00E0]
        //   f = 0  :  dictates call type
        //   x = 0
        //   y = E
        //   n = 0
        //  nn = E0
        // nnn = 0E0

        // OUR TARGET: COSMAC VIP CHIP-8
        switch (instruction.opcode) {
            case opcode::Opcode::CLS:        return "CLS";
            case opcode::Opcode::RET:        return "RET";
            case opcode::Opcode::SYS_ADDR:   return "SYS " + to_hex_string(instruction.nnn, 3);
            case opcode::Opcode::JP_ADDR:    return "JP " + to_hex_string(instruction.nnn, 3);
            case opcode::Opcode::CALL_ADDR:  return "CALL " + to_hex_string(instruction.nnn, 3);

            // SE / SNE
            case opcode::Opcode::SE_V_B:     return "SE V" + to_register_string(instruction.x) + ", " + to_hex_string(instruction.nn, 2);
            case opcode::Opcode::SNE_V_B:    return "SNE V" + to_register_string(instruction.x) + ", " + to_hex_string(instruction.nn, 2);
            case opcode::Opcode::SE_V_V:     return "SE V" + to_register_string(instruction.x) + ", V" + to_register_string(instruction.y);
            case opcode::Opcode::SNE_V_V:    return "SNE V" + to_register_string(instruction.x) + ", V" + to_register_string(instruction.y);

            // LD / ADD immediates
            case opcode::Opcode::LD_V_B:     return "LD V" + to_register_string(instruction.x) + ", " + to_hex_string(instruction.nn, 2);
            case opcode::Opcode::ADD_V_B:    return "ADD V" + to_register_string(instruction.x) + ", " + to_hex_string(instruction.nn, 2);

            // 0x8 ALU operations
            case opcode::Opcode::LD_V_V:     return "LD V" + to_register_string(instruction.x) + ", V" + to_register_string(instruction.y);
            case opcode::Opcode::OR_V_V:     return "OR V" + to_register_string(instruction.x) + ", V" + to_register_string(instruction.y);
            case opcode::Opcode::AND_V_V:    return "AND V" + to_register_string(instruction.x) + ", V" + to_register_string(instruction.y);
            case opcode::Opcode::XOR_V_V:    return "XOR V" + to_register_string(instruction.x) + ", V" + to_register_string(instruction.y);
            case opcode::Opcode::ADD_V_V:    return "ADD V" + to_register_string(instruction.x) + ", V" + to_register_string(instruction.y);
            case opcode::Opcode::SUB_V_V:    return "SUB V" + to_register_string(instruction.x) + ", V" + to_register_string(instruction.y);
            case opcode::Opcode::SHR_V:      return "SHR V" + to_register_string(instruction.x) + ", V" + to_register_string(instruction.y);
            case opcode::Opcode::SUBN_V_V:   return "SUBN V" + to_register_string(instruction.x) + ", V" + to_register_string(instruction.y);
            case opcode::Opcode::SHL_V:      return "SHL V" + to_register_string(instruction.x) + ", V" + to_register_string(instruction.y);

            // Flow / Random / Graphics
            case opcode::Opcode::LD_I_ADDR:  return "LD I, " + to_hex_string(instruction.nnn, 3);
            case opcode::Opcode::JP_V_ADDR:  return "JP V0, " + to_hex_string(instruction.nnn, 3);
            case opcode::Opcode::RND_V_B:    return "RND V" + to_register_string(instruction.x) + ", " + to_hex_string(instruction.nn, 2);
            case opcode::Opcode::DRW_V_V:    return "DRW V" + to_register_string(instruction.x) + ", V" + to_register_string(instruction.y) + ", " + to_hex_string(instruction.n, 1);

            // Key inputs
            case opcode::Opcode::SKP_V:      return "SKP V" + to_register_string(instruction.x);
            case opcode::Opcode::SKNP_V:     return "SKNP V" + to_register_string(instruction.x);

            // 0xF Timer / Memory operations
            case opcode::Opcode::LD_V_DT:    return "LD V" + to_register_string(instruction.x) + ", DT";
            case opcode::Opcode::LD_V_K:     return "LD V" + to_register_string(instruction.x) + ", K";
            case opcode::Opcode::LD_DT_V:    return "LD DT, V" + to_register_string(instruction.x);
            case opcode::Opcode::LD_ST_V:    return "LD ST, V" + to_register_string(instruction.x);
            case opcode::Opcode::ADD_I_V:    return "ADD I, V" + to_register_string(instruction.x);
            case opcode::Opcode::LD_F_V:     return "LD F, V" + to_register_string(instruction.x);
            case opcode::Opcode::LD_B_V:     return "LD B, V" + to_register_string(instruction.x);
            case opcode::Opcode::LD_I_V:     return "LD [I], V" + to_register_string(instruction.x);
            case opcode::Opcode::LD_V_I:     return "LD V" + to_register_string(instruction.x) + ", [I]";

            // invalid
            case opcode::Opcode::INVALID: return "???";
        }
        return "???";
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
        oss << std::uppercase << std::hex << static_cast<int>(val);
        std::string formatted = oss.str();

        return formatted;
    }

    // takes in uint16_t, and width; returns hex friendly string
    std::string to_hex_string(uint16_t value, int width) {
        std::ostringstream oss;

        oss << std::uppercase << std::hex << std::setw(width) << std::setfill('0') << value;
        std::string formatted = oss.str();

        return formatted;
    }
}