#include <cstdint>
#include <array>
#include <string>


constexpr uint16_t MEMORY_SIZE = 4096;
constexpr uint16_t START_ADDRESS_OFFSET = 0x200;

class chip8 {
    public:
        chip8(const std::string& path);
        // populates memory buffer
        bool load_rom_into_memory();

    private:

        std::string rom_path;
        std::array<uint8_t, MEMORY_SIZE> memory = {}; // fixed memory size 4096
};