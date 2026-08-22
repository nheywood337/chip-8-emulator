#include <gtest/gtest.h>
#include <vector>
#include "chip8.h"

class Chip8Test : public ::testing::Test {};

// Setup & Memory Tests
// ----------------------------------------------------------------------------

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

// Fetching
// ----------------------------------------------------------------------------

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

    // Past the end of RAM
    EXPECT_THROW(vm.fetch(), chip8_error);
}

// Execution
// ----------------------------------------------------------------------------

// 1NNN
TEST_F(Chip8Test, ExecuteJMP_ADDR) {
    std::vector<uint8_t> rom = {0x11, 0x11};
    chip8 vm(rom);

    opcode::Instruction instr = opcode::decode(vm.fetch());
    vm.execute(instr);

    EXPECT_EQ(vm.get_program_counter(), 0x111);
}

// 6XNN
TEST_F(Chip8Test, ExecuteLD_V_B) {
    std::vector<uint8_t> rom = {0x62, 0x05};
    chip8 vm(rom);

    opcode::Instruction instr = opcode::decode(vm.fetch());
    vm.execute(instr);

    EXPECT_EQ(vm.get_register(instr.x), instr.nn);
}

// 7XNN
TEST_F(Chip8Test, ExecuteADD_V_B) {
    std::vector<uint8_t> rom = {0x62, 0xFF, 0x72, 0x05};
    chip8 vm(rom);

    // Set V2 = 255
    opcode::Instruction instr = opcode::decode(vm.fetch());
    vm.execute(instr);

    // Should wrap around to 4
    instr = opcode::decode(vm.fetch());
    vm.execute(instr);

    EXPECT_EQ(vm.get_register(0x2), 0x04);
}

// 2NNN
TEST_F(Chip8Test, ExecuteCALL_ADDR) {
    std::vector<uint8_t> rom = {0x23, 0x45};
    chip8 vm(rom);

    opcode::Instruction instr = opcode::decode(vm.fetch());
    vm.execute(instr);

    EXPECT_EQ(vm.get_program_counter(), 0x345);
}

// 00EE
TEST_F(Chip8Test, ExecuteRET) {
    std::vector<uint8_t> rom = {
        0x22, 0x04, // CALL 0x204
        0x00, 0x00, // NOP
        0x00, 0xEE  // RET
    };
    chip8 vm(rom);

    // CALL 0x204
    opcode::Instruction call_instr = opcode::decode(vm.fetch());
    vm.execute(call_instr);
    EXPECT_EQ(vm.get_program_counter(), 0x204);

    // Back to 0x202
    opcode::Instruction ret_instr = opcode::decode(vm.fetch());
    vm.execute(ret_instr);
    EXPECT_EQ(vm.get_program_counter(), 0x202);
}

TEST_F(Chip8Test, NestedSubroutineCallsUnwindInLIFOOrder) {
    std::vector<uint8_t> rom = {
        0x22, 0x06, // CALL 0x206
        0x00, 0x00,
        0x00, 0x00,
        0x22, 0x0A, // CALL 0x20A
        0x00, 0xEE, // RET
        0x00, 0xEE  // RET
    };
    chip8 vm(rom);

    // First call, return address is 0x202
    vm.execute(opcode::decode(vm.fetch()));
    EXPECT_EQ(vm.get_program_counter(), 0x206);

    // Second call, return address is 0x208
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
    std::vector<uint8_t> rom = {0x22, 0x00}; // Recursion loop
    chip8 vm(rom);

    // Max out the 16 stack frames
    for (int i = 0; i < 16; ++i) {
        opcode::Instruction instr = opcode::decode(vm.fetch());
        vm.execute(instr);
    }

    // 17th call should blow up
    opcode::Instruction overflow_instr = opcode::decode(vm.fetch());
    EXPECT_THROW(vm.execute(overflow_instr), chip8_error);
}

TEST_F(Chip8Test, ExecuteRET_ThrowsOnStackUnderflow) {
    std::vector<uint8_t> rom = {0x00, 0xEE};
    chip8 vm(rom);

    opcode::Instruction instr = opcode::decode(vm.fetch());
    EXPECT_THROW(vm.execute(instr), chip8_error);
}

// 3XNN (Values match -> skip)
TEST_F(Chip8Test, ExecuteSE_V_B_EqualNN) {
    std::vector<uint8_t> rom = {0x61, 0x10, 0x31, 0x10};
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    uint16_t pc_before_skip = vm.get_program_counter();

    vm.execute(opcode::decode(vm.fetch()));
    EXPECT_EQ(vm.get_program_counter(), pc_before_skip + 4);
}

// 3XNN (Values don't match -> don't skip)
TEST_F(Chip8Test, ExecuteSE_V_B_NotEqualNN) {
    std::vector<uint8_t> rom = {0x61, 0x20, 0x31, 0x10};
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    uint16_t pc_before_skip = vm.get_program_counter();

    vm.execute(opcode::decode(vm.fetch()));
    EXPECT_EQ(vm.get_program_counter(), pc_before_skip + 2);
}

// 4XNN (Values match -> don't skip)
TEST_F(Chip8Test, ExecuteSNE_V_B_EqualNN) {
    std::vector<uint8_t> rom = {0x61, 0x10, 0x41, 0x10};
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    uint16_t pc_before_skip = vm.get_program_counter();

    vm.execute(opcode::decode(vm.fetch()));
    EXPECT_EQ(vm.get_program_counter(), pc_before_skip + 2);
}

// 4XNN (Values don't match -> skip)
TEST_F(Chip8Test, ExecuteSNE_V_B_NotEqualNN) {
    std::vector<uint8_t> rom = {0x61, 0x20, 0x41, 0x10};
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    uint16_t pc_before_skip = vm.get_program_counter();

    vm.execute(opcode::decode(vm.fetch()));
    EXPECT_EQ(vm.get_program_counter(), pc_before_skip + 4);
}

// 5XY0 (Equal -> skip)
TEST_F(Chip8Test, ExecuteSE_V_V_Equal) {
    std::vector<uint8_t> rom = {
        0x61, 0x10, 
        0x62, 0x10, 
        0x51, 0x20
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));
    
    uint16_t pc_before_skip = vm.get_program_counter();

    vm.execute(opcode::decode(vm.fetch()));
    
    EXPECT_EQ(vm.get_program_counter(), pc_before_skip + 4);
}

// 5XY0 (Not equal -> don't skip)
TEST_F(Chip8Test, ExecuteSE_V_V_NotEqual) {
    std::vector<uint8_t> rom = {
        0x61, 0x10, 
        0x62, 0x20, 
        0x51, 0x20
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));
    
    uint16_t pc_before_skip = vm.get_program_counter();

    vm.execute(opcode::decode(vm.fetch()));
    
    EXPECT_EQ(vm.get_program_counter(), pc_before_skip + 2);
}

// 9XY0 (Equal -> don't skip)
TEST_F(Chip8Test, ExecuteSNE_V_V_Equal) {
    std::vector<uint8_t> rom = {
        0x61, 0x10,
        0x62, 0x10,
        0x91, 0x20
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));
    
    uint16_t pc_before_skip = vm.get_program_counter();

    vm.execute(opcode::decode(vm.fetch()));
    EXPECT_EQ(vm.get_program_counter(), pc_before_skip + 2);
}

// 9XY0 (Not equal -> skip)
TEST_F(Chip8Test, ExecuteSNE_V_V_NotEqual) {
    std::vector<uint8_t> rom = {
        0x61, 0x10,
        0x62, 0x20,
        0x91, 0x20
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));
    
    uint16_t pc_before_skip = vm.get_program_counter();

    vm.execute(opcode::decode(vm.fetch()));
    EXPECT_EQ(vm.get_program_counter(), pc_before_skip + 4);
}

// 8XY0
TEST_F(Chip8Test, ExecuteLD_V_V) {
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

// 8XY1
TEST_F(Chip8Test, ExecuteOR_V_V) {
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

// 8XY2
TEST_F(Chip8Test, ExecuteAND_V_V) {
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

// 8XY3
TEST_F(Chip8Test, ExecuteXOR_V_V) {
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

// 8XY4
TEST_F(Chip8Test, ExecuteADD_V_V_Overflow) {
    std::vector<uint8_t> rom = {
        0x61, 0xFF,
        0x62, 0x01,
        0x81, 0x24
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));

    // 255 + 1 rolls over to 0
    EXPECT_EQ(vm.get_register(0x1), 0x00);

    // VF handles the carry bit
    EXPECT_EQ(vm.get_register(VF), 1);
}

// 8XY5
TEST_F(Chip8Test, ExecuteSUB_V_V_Underflow) {
    std::vector<uint8_t> rom = {
        0x61, 0x05,
        0x62, 0x0A,
        0x81, 0x25
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));

    // 5 - 10 wraps to 251
    EXPECT_EQ(vm.get_register(0x1), 0xFB);

    // VF = 0 means we had to borrow
    EXPECT_EQ(vm.get_register(VF), 0);
}

// 8FY4 (writing directly to VF)
TEST_F(Chip8Test, ExecuteADD_V_V_TargetIsVF) {
    std::vector<uint8_t> rom = {
        0x6F, 0xE0,
        0x62, 0x30,
        0x8F, 0x24
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));

    // Carry check happens last, so carry bit overrides the addition result in VF
    EXPECT_EQ(vm.get_register(VF), 1);
}

// 8FY5 (writing directly to VF)
TEST_F(Chip8Test, ExecuteSUB_V_V_TargetIsVF) {
    std::vector<uint8_t> rom = {
        0x6F, 0x0A,
        0x62, 0x05,
        0x8F, 0x25
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));

    // Borrow flag gets written last, overwriting the math result in VF
    EXPECT_EQ(vm.get_register(VF), 1);
}

// 8FY1 (target is VF)
TEST_F(Chip8Test, ExecuteOR_V_V_TargetIsVF) {
    std::vector<uint8_t> rom = {
        0x6F, 0xF0,
        0x62, 0x0F,
        0x8F, 0x21
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));

    // Logical operations reset VF back to 0
    EXPECT_EQ(vm.get_register(VF), 0);
}

// 8FY2 (target is VF)
TEST_F(Chip8Test, ExecuteAND_V_V_TargetIsVF) {
    std::vector<uint8_t> rom = {
        0x6F, 0xFF,
        0x62, 0xFF,
        0x8F, 0x22
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));

    // Logical operations reset VF back to 0
    EXPECT_EQ(vm.get_register(VF), 0);
}

// 8FY3 (target is VF)
TEST_F(Chip8Test, ExecuteXOR_V_V_TargetIsVF) {
    std::vector<uint8_t> rom = {
        0x6F, 0xFF,
        0x62, 0x00,
        0x8F, 0x23
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));

    // Logical operations reset VF back to 0
    EXPECT_EQ(vm.get_register(VF), 0);
}

// 8XY6 (LSB is 1)
TEST_F(Chip8Test, ExecuteSHR_V_LSB_One) {
    std::vector<uint8_t> rom = {
        0x62, 0x03,
        0x81, 0x26
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));

    EXPECT_EQ(vm.get_register(0x1), 0x01);
    EXPECT_EQ(vm.get_register(VF), 1);
}

// 8XY6 (LSB is 0)
TEST_F(Chip8Test, ExecuteSHR_V_LSB_Zero) {
    std::vector<uint8_t> rom = {
        0x62, 0x04,
        0x81, 0x26
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));

    EXPECT_EQ(vm.get_register(0x1), 0x02);
    EXPECT_EQ(vm.get_register(VF), 0);
}

TEST_F(Chip8Test, ExecuteSUBN_V_V_NoUnderflow) {
    std::vector<uint8_t> rom = {0x61, 0x05, 0x62, 0x0A, 0x81, 0x27};
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));

    EXPECT_EQ(vm.get_register(0x1), 0x05);
    EXPECT_EQ(vm.get_register(VF), 1);
}

TEST_F(Chip8Test, ExecuteSUBN_V_V_Underflow) {
    std::vector<uint8_t> rom = {0x61, 0x0A, 0x62, 0x05, 0x81, 0x27};
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));

    EXPECT_EQ(vm.get_register(0x1), static_cast<uint8_t>(5 - 10));
    EXPECT_EQ(vm.get_register(VF), 0);
}

TEST_F(Chip8Test, ExecuteSUBN_V_V_TargetIsVF) {
    std::vector<uint8_t> rom = {0x6F, 0x05, 0x62, 0x0A, 0x8F, 0x27};
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));

    // Borrow flag overwrites subtraction result
    EXPECT_EQ(vm.get_register(VF), 1);
}

TEST_F(Chip8Test, ExecuteSHL_V_MSB_One) {
    std::vector<uint8_t> rom = {0x62, 0x85, 0x81, 0x2E};
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));

    EXPECT_EQ(vm.get_register(0x1), 0x0A);
    EXPECT_EQ(vm.get_register(VF), 1);
}

TEST_F(Chip8Test, ExecuteSHL_V_MSB_Zero) {
    std::vector<uint8_t> rom = {0x62, 0x45, 0x81, 0x2E};
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));

    EXPECT_EQ(vm.get_register(0x1), 0x8A);
    EXPECT_EQ(vm.get_register(VF), 0);
}

TEST_F(Chip8Test, ExecuteSHL_V_TargetIsVF) {
    std::vector<uint8_t> rom = {0x6F, 0x85, 0x8F, 0xFE};
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));
    vm.execute(opcode::decode(vm.fetch()));

    EXPECT_EQ(vm.get_register(VF), 1);
}

TEST_F(Chip8Test, ExecuteLD_I_ADDR) {
    std::vector<uint8_t> rom = {0xA1, 0x11};
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch()));

    EXPECT_EQ(vm.get_I(), 0x111);
}

// BNNN
TEST_F(Chip8Test, ExecuteJP_V_ADDR) {
    std::vector<uint8_t> rom = {0x60, 0x0A, 0xB1, 0x11};
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch())); // V0 = 10
    vm.execute(opcode::decode(vm.fetch())); // Jump to 0x111 + V0

    EXPECT_EQ(vm.get_program_counter(), 0x011B);
}

// CXNN
TEST_F(Chip8Test, ExecuteRND_V_B) {
    std::vector<uint8_t> rom = {0xC1, 0x0F};
    chip8 vm(rom);  

    vm.execute(opcode::decode(vm.fetch()));
    EXPECT_EQ(vm.get_register(0x1) & 0x0F, vm.get_register(0x1));
}

TEST_F(Chip8Test, ExecuteDRW_DrawsSpriteToDisplay) {
    std::vector<uint8_t> rom = {
        0x61, 0x00, // V1 = 0
        0x62, 0x00, // V2 = 0
        0xA2, 0x08, // Set I to 0x208
        0xD1, 0x21, // Draw at (V1, V2)
        0xA0        // Sprite data (0b10100000) stored at 0x208
    };
    chip8 vm(rom);

    for (int i = 0; i < 4; ++i) {
        vm.execute(opcode::decode(vm.fetch()));
    }

    const auto& display = vm.get_display();
    EXPECT_EQ(display.at(0), 1);
    EXPECT_EQ(display.at(1), 0);
    EXPECT_EQ(display.at(2), 1);
    EXPECT_EQ(vm.get_register(VF), 0);
}

TEST_F(Chip8Test, ExecuteDRW_DetectsCollisionAndTogglesPixelsOff) {
    std::vector<uint8_t> rom = {
        0x61, 0x00, // V1 = 0
        0x62, 0x00, // V2 = 0
        0xA2, 0x0A, // Set I to 0x20A
        0xD1, 0x21, // First draw
        0xD1, 0x21, // Second draw (XOR over same spot)
        0x80        // Sprite data (0b10000000) stored at 0x20A
    };
    chip8 vm(rom);

    for (int i = 0; i < 4; ++i) {
        vm.execute(opcode::decode(vm.fetch()));
    }
    EXPECT_EQ(vm.get_display().at(0), 1);
    EXPECT_EQ(vm.get_register(VF), 0);

    // Draw second time, pixel turns off and sets collision flag
    vm.execute(opcode::decode(vm.fetch()));
    EXPECT_EQ(vm.get_display().at(0), 0);
    EXPECT_EQ(vm.get_register(VF), 1);
}

TEST_F(Chip8Test, ExecuteCLS) {
    std::vector<uint8_t> rom = {
        0x61, 0x00,
        0x62, 0x00,
        0xA2, 0x0A,
        0xD1, 0x21,
        0x00, 0xE0,
        0x80
    };
    chip8 vm(rom);

    // Draw the pixel first
    for (int i = 0; i < 4; ++i) {
        vm.execute(opcode::decode(vm.fetch()));
    }
    EXPECT_EQ(vm.get_display().at(0), 1);

    // Execute CLS
    vm.execute(opcode::decode(vm.fetch()));

    // Verify all screen pixels are cleared
    for (const auto& pixel : vm.get_display()) {
        EXPECT_EQ(pixel, 0);
    }
}