#include <gtest/gtest.h>
#include "chip8.h"
#include <vector>

class Chip8Test : public ::testing::Test {
};

TEST_F(Chip8Test, ConstructorLoadsRomAtCorrectOffset) {
    std::vector<uint8_t> rom = {0x12, 0x34, 0x56, 0x78};
    chip8 vm(rom);

    EXPECT_EQ(vm.fetch(), 0x1234);
    EXPECT_EQ(vm.fetch(), 0x5678);
}

TEST_F(Chip8Test, ConstructorThrowsWhenRomIsTooLarge) {
    size_t max_allowed = MEMORY_SIZE - START_ADDRESS_OFFSET;
    std::vector<uint8_t> oversized_rom(max_allowed + 1, 0xAA);

    EXPECT_THROW(chip8 vm(oversized_rom), chip8_error);
}

TEST_F(Chip8Test, ConstructorAcceptsMaximumValidRomSize) {
    size_t max_allowed = MEMORY_SIZE - START_ADDRESS_OFFSET;
    std::vector<uint8_t> max_rom(max_allowed, 0xFF);

    chip8 vm(max_rom);
}


TEST_F(Chip8Test, FetchCombinesBigEndianOpcodeAndAdvancesPC) {
    std::vector<uint8_t> rom = {0x00, 0xE0, 0x12, 0x00};
    chip8 vm(rom);

    EXPECT_EQ(vm.fetch(), 0x00E0);
    EXPECT_EQ(vm.fetch(), 0x1200);
}

TEST_F(Chip8Test, FetchThrowsWhenBothBytesAreOutOfBounds) {
    size_t max_allowed = MEMORY_SIZE - START_ADDRESS_OFFSET;
    std::vector<uint8_t> rom(max_allowed, 0x00);
    chip8 vm(rom);

    for (size_t i = 0; i < max_allowed / 2; ++i) {
        vm.fetch();
    }

    // Both hi and lo fall outside valid memory range
    EXPECT_THROW(vm.fetch(), chip8_error);
}

TEST_F(Chip8Test, ExecuteLD_V_B) {
    std::vector<uint8_t> rom = {0x62, 0x05};
    chip8 vm(rom);

    EXPECT_EQ(vm.execute(opcode::decode(vm.fetch())), );
}