#include <cstdint>
#include <array>
#include <string>


const static uint16_t MEMORY_SIZE = 4096;
const static uint16_t START_ADDRESS = 0x200;

class chip8 {
    public:
        bool load_rom(const std::string& path);

    private:


        std::array<uint8_t, MEMORY_SIZE> memory = {0}; // fixed memory size 4096
};