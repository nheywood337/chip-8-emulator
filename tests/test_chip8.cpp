#include <gtest/gtest.h>
#include <vector>
#include "chip8.h"

class Chip8Test : public ::testing::Test {};

// ============================================================================
// Initialization & Memory Setup
// ============================================================================

TEST_F(Chip8Test, ConstructorLoadsRomAtCorrectOffset) {
    std::vector<uint8_t> rom = {0x12, 0x34, 0x56, 0x78};
    chip8 vm(rom);

    EXPECT_EQ(vm.fetch(), 0x1234);
    EXPECT_EQ(vm.fetch(), 0x5678);
}

TEST_F(Chip8Test, ConstructorAcceptsMaximumValidRomSize) {
    size_t max_allowed = MEMORY_SIZE - START_ADDRESS_OFFSET;
    std::vector<uint8_t> max_rom(max_allowed, 0xFF);

    chip8 vm(max_rom);
}

TEST_F(Chip8Test, ConstructorThrowsWhenRomIsTooLarge) {
    size_t max_allowed = MEMORY_SIZE - START_ADDRESS_OFFSET;
    std::vector<uint8_t> oversized_rom(max_allowed + 1, 0xAA);

    EXPECT_THROW(chip8 vm(oversized_rom), chip8_error);
}

// ============================================================================
// Instruction Fetching
// ============================================================================

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

    // Out of memory bounds
    EXPECT_THROW(vm.fetch(), chip8_error);
}

// ============================================================================
// Instruction Execution
// ============================================================================

// 1NNN: Jump to NNN
TEST_F(Chip8Test, ExecuteJMP_ADDR) {
    std::vector<uint8_t> rom = {0x11, 0x11};
    chip8 vm(rom);

    opcode::Instruction instr = opcode::decode(vm.fetch());
    vm.execute(instr);

    EXPECT_EQ(vm.get_program_counter(), 0x111);
}

// 6XNN: Set VX = NN
TEST_F(Chip8Test, ExecuteLD_V_B) {
    std::vector<uint8_t> rom = {0x62, 0x05};
    chip8 vm(rom);

    opcode::Instruction instr = opcode::decode(vm.fetch());
    vm.execute(instr);

    EXPECT_EQ(vm.get_register(instr.x), instr.nn);
}

// 7XNN: Add NN to VX (wraps on 8-bit overflow)
TEST_F(Chip8Test, ExecuteADD_V_B) {
    std::vector<uint8_t> rom = {0x62, 0xFF, 0x72, 0x05};
    chip8 vm(rom);

    // Set V2 = 0xFF
    opcode::Instruction instr = opcode::decode(vm.fetch());
    vm.execute(instr);

    // V2 += 0x05 (should overflow to 0x04)
    instr = opcode::decode(vm.fetch());
    vm.execute(instr);

    EXPECT_EQ(vm.get_register(0x2), 0x04);
}

// 2NNN: Call subroutine
TEST_F(Chip8Test, ExecuteCALL_ADDR) {
    std::vector<uint8_t> rom = {0x23, 0x45};
    chip8 vm(rom);

    opcode::Instruction instr = opcode::decode(vm.fetch());
    vm.execute(instr);

    EXPECT_EQ(vm.get_program_counter(), 0x345);
}

// 00EE: Return from subroutine
TEST_F(Chip8Test, ExecuteRET) {
    std::vector<uint8_t> rom = {
        0x22, 0x04, // 0x200: CALL 0x204
        0x00, 0x00, // 0x202: NOP
        0x00, 0xEE  // 0x204: RET
    };
    chip8 vm(rom);

    // CALL 0x204
    opcode::Instruction call_instr = opcode::decode(vm.fetch());
    vm.execute(call_instr);
    EXPECT_EQ(vm.get_program_counter(), 0x204);

    // RET to 0x202
    opcode::Instruction ret_instr = opcode::decode(vm.fetch());
    vm.execute(ret_instr);
    EXPECT_EQ(vm.get_program_counter(), 0x202);
}

TEST_F(Chip8Test, NestedSubroutineCallsUnwindInLIFOOrder) {
    std::vector<uint8_t> rom = {
        0x22, 0x06, // 0x200: CALL 0x206
        0x00, 0x00, // 0x202
        0x00, 0x00, // 0x204
        0x22, 0x0A, // 0x206: CALL 0x20A
        0x00, 0xEE, // 0x208: RET
        0x00, 0xEE  // 0x20A: RET
    };
    chip8 vm(rom);

    // Call 1 (return addr: 0x202)
    vm.execute(opcode::decode(vm.fetch()));
    EXPECT_EQ(vm.get_program_counter(), 0x206);

    // Call 2 (return addr: 0x208)
    vm.execute(opcode::decode(vm.fetch()));
    EXPECT_EQ(vm.get_program_counter(), 0x20A);

    // Return to 0x208
    vm.execute(opcode::decode(vm.fetch()));
    EXPECT_EQ(vm.get_program_counter(), 0x208);

    // Return to 0x202
    vm.execute(opcode::decode(vm.fetch()));
    EXPECT_EQ(vm.get_program_counter(), 0x202);
}

TEST_F(Chip8Test, ExecuteCALL_ADDR_ThrowsOnStackOverflow) {
    std::vector<uint8_t> rom = {0x22, 0x00}; // Infinite recursion
    chip8 vm(rom);

    // Fill 16-level stack
    for (int i = 0; i < 16; ++i) {
        opcode::Instruction instr = opcode::decode(vm.fetch());
        vm.execute(instr);
    }

    // 17th call exceeds depth limit
    opcode::Instruction overflow_instr = opcode::decode(vm.fetch());
    EXPECT_THROW(vm.execute(overflow_instr), chip8_error);
}

TEST_F(Chip8Test, ExecuteRET_ThrowsOnStackUnderflow) {
    std::vector<uint8_t> rom = {0x00, 0xEE};
    chip8 vm(rom);

    opcode::Instruction instr = opcode::decode(vm.fetch());
    EXPECT_THROW(vm.execute(instr), chip8_error);
}