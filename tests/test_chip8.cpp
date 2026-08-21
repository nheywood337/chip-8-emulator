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

// 3XNN: Skip next instruction if VX == NN (Equal)
TEST_F(Chip8Test, ExecuteSE_V_B_EqualNN) {
    std::vector<uint8_t> rom = {0x61, 0x10, 0x31, 0x10};
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    uint16_t pc_before_skip = vm.get_program_counter();

    vm.execute(opcode::decode(vm.fetch()));
    EXPECT_EQ(vm.get_program_counter(), pc_before_skip + 4);
}

// 3XNN: Skip next instruction if VX == NN (Not Equal)
TEST_F(Chip8Test, ExecuteSE_V_B_NotEqualNN) {
    std::vector<uint8_t> rom = {0x61, 0x20, 0x31, 0x10};
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    uint16_t pc_before_skip = vm.get_program_counter();

    vm.execute(opcode::decode(vm.fetch()));
    EXPECT_EQ(vm.get_program_counter(), pc_before_skip + 2);
}

// 4XNN skip next opcode if vX != NN
TEST_F(Chip8Test, ExecuteSNE_V_B_EqualNN) {
    std::vector<uint8_t> rom = {0x61, 0x10, 0x41, 0x10};
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    uint16_t pc_before_skip = vm.get_program_counter();

    vm.execute(opcode::decode(vm.fetch()));
    EXPECT_EQ(vm.get_program_counter(), pc_before_skip + 2);
}

// 4XNN skip next opcode if vX != NN
TEST_F(Chip8Test, ExecuteSNE_V_B_NotEqualNN) {
    std::vector<uint8_t> rom = {0x61, 0x20, 0x41, 0x10};
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    uint16_t pc_before_skip = vm.get_program_counter();

    vm.execute(opcode::decode(vm.fetch()));
    EXPECT_EQ(vm.get_program_counter(), pc_before_skip + 4);
}

// 5XY0: Skip next opcode if VX == VY (Equal -> Skip)
TEST_F(Chip8Test, ExecuteSE_V_V_Equal) {
    // 0x6110 -> Set V1 = 0x10
    // 0x6210 -> Set V2 = 0x10
    // 0x5120 -> Skip if V1 == V2 (Equal -> Should skip)
    std::vector<uint8_t> rom = {
        0x61, 0x10, 
        0x62, 0x10, 
        0x51, 0x20
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch())); // Set V1 = 0x10
    vm.execute(opcode::decode(vm.fetch())); // Set V2 = 0x10
    
    uint16_t pc_before_skip = vm.get_program_counter(); // 0x204

    vm.execute(opcode::decode(vm.fetch())); // Execute 5120
    
    // Fetch (+2) + Skip (+2) = +4 total
    EXPECT_EQ(vm.get_program_counter(), pc_before_skip + 4);
}

// 5XY0: Skip next opcode if VX == VY (Not Equal -> No Skip)
TEST_F(Chip8Test, ExecuteSE_V_V_NotEqual) {
    // 0x6110 -> Set V1 = 0x10
    // 0x6220 -> Set V2 = 0x20
    // 0x5120 -> Skip if V1 == V2 (Not equal -> Should NOT skip)
    std::vector<uint8_t> rom = {
        0x61, 0x10, 
        0x62, 0x20, 
        0x51, 0x20
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch())); // Set V1 = 0x10
    vm.execute(opcode::decode(vm.fetch())); // Set V2 = 0x20
    
    uint16_t pc_before_skip = vm.get_program_counter(); // 0x204

    vm.execute(opcode::decode(vm.fetch())); // Execute 5120
    
    // Normal step (+2)
    EXPECT_EQ(vm.get_program_counter(), pc_before_skip + 2);
}

// 9XY0: Skip next opcode if VX != VY (Equal -> No Skip)
TEST_F(Chip8Test, ExecuteSNE_V_V_Equal) {
    // 0x6110 -> Set V1 = 0x10
    // 0x6210 -> Set V2 = 0x10
    // 0x9120 -> Skip if V1 != V2
    std::vector<uint8_t> rom = {
        0x61, 0x10,
        0x62, 0x10,
        0x91, 0x20
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));
    
    uint16_t pc_before_skip = vm.get_program_counter(); // 0x204

    // Equal values -> Do NOT skip (+2)
    vm.execute(opcode::decode(vm.fetch()));
    EXPECT_EQ(vm.get_program_counter(), pc_before_skip + 2);
}

// 9XY0: Skip next opcode if VX != VY (Not Equal -> Skip)
TEST_F(Chip8Test, ExecuteSNE_V_V_NotEqual) {
    // 0x6110 -> Set V1 = 0x10
    // 0x6220 -> Set V2 = 0x20
    // 0x9120 -> Skip if V1 != V2
    std::vector<uint8_t> rom = {
        0x61, 0x10,
        0x62, 0x20,
        0x91, 0x20
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));
    
    uint16_t pc_before_skip = vm.get_program_counter(); // 0x204

    // Not equal values -> Skip (+4)
    vm.execute(opcode::decode(vm.fetch()));
    EXPECT_EQ(vm.get_program_counter(), pc_before_skip + 4);
}

// 8XY0: Set VX = VY
TEST_F(Chip8Test, ExecuteLD_V_V) {
    // 0x6110 -> Set V1 = 0x10
    // 0x6220 -> Set V2 = 0x20
    // 0x8120 -> Set V1 = V2
    std::vector<uint8_t> rom = {
        0x61, 0x10,
        0x62, 0x20,
        0x81, 0x20
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));

    EXPECT_EQ(vm.get_register(0x1), 0x20);
}

// 8XY1: Set VX = VX | VY
TEST_F(Chip8Test, ExecuteOR_V_V) {
    // 0x610F -> Set V1 = 0x0F (00001111)
    // 0x62F0 -> Set V2 = 0xF0 (11110000)
    // 0x8121 -> V1 |= V2     (11111111 = 0xFF)
    std::vector<uint8_t> rom = {
        0x61, 0x0F,
        0x62, 0xF0,
        0x81, 0x21
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));

    EXPECT_EQ(vm.get_register(0x1), 0xFF);
}

// 8XY2: Set VX = VX & VY
TEST_F(Chip8Test, ExecuteAND_V_V) {
    // 0x61FF -> Set V1 = 0xFF (11111111)
    // 0x620F -> Set V2 = 0x0F (00001111)
    // 0x8122 -> V1 &= V2     (00001111 = 0x0F)
    std::vector<uint8_t> rom = {
        0x61, 0xFF,
        0x62, 0x0F,
        0x81, 0x22
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));

    EXPECT_EQ(vm.get_register(0x1), 0x0F);
}

// 8XY3: Set VX = VX ^ VY
TEST_F(Chip8Test, ExecuteXOR_V_V) {
    // 0x61FF -> Set V1 = 0xFF (11111111)
    // 0x620F -> Set V2 = 0x0F (00001111)
    // 0x8123 -> V1 ^= V2     (11110000 = 0xF0)
    std::vector<uint8_t> rom = {
        0x61, 0xFF,
        0x62, 0x0F,
        0x81, 0x23
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));

    EXPECT_EQ(vm.get_register(0x1), 0xF0);
}

// 8XY4: ADD VX, VY (Set VX = VX + VY, VF = carry)
TEST_F(Chip8Test, ExecuteADD_V_V_Overflow) {
    // 0x61FF -> Set V1 = 0xFF (255)
    // 0x6201 -> Set V2 = 0x01 (1)
    // 0x8124 -> V1 += V2 (Overflows to 0x00, sets VF = 1)
    std::vector<uint8_t> rom = {
        0x61, 0xFF,
        0x62, 0x01,
        0x81, 0x24
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch())); // V1 = 0xFF
    vm.execute(opcode::decode(vm.fetch())); // V2 = 0x01
    vm.execute(opcode::decode(vm.fetch())); // V1 += V2

    // Check V1 result (0xFF + 0x01 wraps around to 0x00)
    EXPECT_EQ(vm.get_register(0x1), 0x00);

    // Check VF (Register 15) flag separately for carry
    EXPECT_EQ(vm.get_register(0xF), 1);
}

// 8XY5: SUB VX, VY (Set VX = VX - VY, VF = NOT borrow)
TEST_F(Chip8Test, ExecuteSUB_V_V_Underflow) {
    // 0x6105 -> Set V1 = 0x05 (5)
    // 0x620A -> Set V2 = 0x0A (10)
    // 0x8125 -> V1 -= V2 (Underflows to 0xFB, sets VF = 0)
    std::vector<uint8_t> rom = {
        0x61, 0x05,
        0x62, 0x0A,
        0x81, 0x25
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch())); // V1 = 0x05
    vm.execute(opcode::decode(vm.fetch())); // V2 = 0x0A
    vm.execute(opcode::decode(vm.fetch())); // V1 -= V2

    // Check V1 result (5 - 10 underflows to 251 / 0xFB)
    EXPECT_EQ(vm.get_register(0x1), 0xFB);

    // Check VF (Register 15) flag separately for borrow (0 means borrow occurred)
    EXPECT_EQ(vm.get_register(0xF), 0);
}

// 8FY4: ADD VF, VY where X = 0xF (Target register is VF itself)
TEST_F(Chip8Test, ExecuteADD_V_V_TargetIsVF) {
    // 0x6FE0 -> Set VF = 0xE0 (224)
    // 0x6230 -> Set V2 = 0x30 (48)
    // 0x8F24 -> VF += V2 (0xE0 + 0x30 = 0x110 -> Wraps to 0x10 with overflow)
    std::vector<uint8_t> rom = {
        0x6F, 0xE0,
        0x62, 0x30,
        0x8F, 0x24
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch())); // VF = 0xE0
    vm.execute(opcode::decode(vm.fetch())); // V2 = 0x30
    vm.execute(opcode::decode(vm.fetch())); // VF += V2

    // Flag write occurs after arithmetic calculation:
    // sum = 0xE0 + 0x30 = 0x110 (272 > 255 -> carry flag = 1)
    // VF gets set to 1 (carry flag), NOT the truncated sum 0x10
    EXPECT_EQ(vm.get_register(0xF), 1);
}

// 8FY5: SUB VF, VY where X = 0xF (Target register is VF itself)
TEST_F(Chip8Test, ExecuteSUB_V_V_TargetIsVF) {
    // 0x6F0A -> Set VF = 0x0A (10)
    // 0x6205 -> Set V2 = 0x05 (5)
    // 0x8F25 -> VF -= V2 (10 - 5 = 5 -> No borrow, VF should become 1)
    std::vector<uint8_t> rom = {
        0x6F, 0x0A,
        0x62, 0x05,
        0x8F, 0x25
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch())); // VF = 0x0A
    vm.execute(opcode::decode(vm.fetch())); // V2 = 0x05
    vm.execute(opcode::decode(vm.fetch())); // VF -= V2

    // diff = 10 - 5 = 5 (>= 0 -> no borrow flag = 1)
    // VF gets set to 1 (no-borrow flag), NOT the arithmetic difference 5
    EXPECT_EQ(vm.get_register(0xF), 1);
}