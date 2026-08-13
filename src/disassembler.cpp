#include "disassembler.h"
#include <string>
#include <iostream>
#include <sstream> // temporary debugging
#include <iomanip> // temporary debugging

namespace disassembler {
    std::vector<std::string> disassemble(const std::vector<uint8_t>& rom_bytes) {
        std::vector<std::string> result;
        
        for (size_t i = 0; i < rom_bytes.size(); i+=2) {
            if (i+1 >= rom_bytes.size()) break;

            uint8_t byte_hi = rom_bytes[i];
            uint8_t byte_lo = rom_bytes[i+1];
            uint16_t combined = (static_cast<uint16_t>(byte_hi) << 8) | static_cast<uint16_t>(byte_lo);    

            // format opcodes
            std::ostringstream oss;
            oss << std::hex << std::setw(4) << std::setfill('0') << combined;
            std::string formatted = oss.str();

            result.push_back(formatted);
        }
        
        return result;
    }


}