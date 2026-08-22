#include <gtest/gtest.h>
#include <vector>
#include "chip8.h"

class Chip8Test : public ::testing::Test {};

// Setup & Memory
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

    // Wraps to 4
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
    std::vector<uint8_t> rom = {0x22, 0x00}; // Infinite recursion
    chip8 vm(rom);

    // Fill all 16 stack slots
    for (int i = 0; i < 16; ++i) {
        opcode::Instruction instr = opcode::decode(vm.fetch());
        vm.execute(instr);
    }

    // 17th call should throw
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

// 3XNN (Values differ -> don't skip)
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

// 4XNN (Values differ -> skip)
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

    // 255 + 1 wraps to 0
    EXPECT_EQ(vm.get_register(0x1), 0x00);

    // Carry bit goes to VF
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

    // Borrow occurred, so VF = 0
    EXPECT_EQ(vm.get_register(VF), 0);
}

// 8FY4 (target is VF)
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

    // Carry flag overwrites addition result
    EXPECT_EQ(vm.get_register(VF), 1);
}

// 8FY5 (target is VF)
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

    // Borrow flag overwrites math result
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

    // Logic ops clear VF to 0
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

    // Logic ops clear VF to 0
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

    // Logic ops clear VF to 0
    EXPECT_EQ(vm.get_register(VF), 0);
}

// 8XY6 (LSB = 1)
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

// 8XY6 (LSB = 0)
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

    // Borrow flag overwrites math result
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
        0xA2, 0x08, // Set I = 0x208
        0xD1, 0x21, // Draw at (V1, V2)
        0xA0        // Sprite data (0b10100000) at 0x208
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
        0xA2, 0x0A, // Set I = 0x20A
        0xD1, 0x21, // Draw 1
        0xD1, 0x21, // Draw 2 (XOR same pixel)
        0x80        // Sprite data (0b10000000) at 0x20A
    };
    chip8 vm(rom);

    for (int i = 0; i < 4; ++i) {
        vm.execute(opcode::decode(vm.fetch()));
    }
    EXPECT_EQ(vm.get_display().at(0), 1);
    EXPECT_EQ(vm.get_register(VF), 0);

    // Second draw turns pixel off and sets collision
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

    // Draw pixel
    for (int i = 0; i < 4; ++i) {
        vm.execute(opcode::decode(vm.fetch()));
    }
    EXPECT_EQ(vm.get_display().at(0), 1);

    // Clear screen
    vm.execute(opcode::decode(vm.fetch()));

    // Make sure everything is cleared
    for (const auto& pixel : vm.get_display()) {
        EXPECT_EQ(pixel, 0);
    }
}

// EX9E & EXA1
// ----------------------------------------------------------------------------

TEST_F(Chip8Test, ExecuteSKP_V_KeyIsPressed_SkipsNextInstruction) {
    std::vector<uint8_t> rom = {0x61, 0x05, 0xE1, 0x9E};
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch())); // V1 = 5
    vm.set_keypad_state(5, true);          // Key 5 pressed
    vm.execute(opcode::decode(vm.fetch())); // SKP V1

    EXPECT_EQ(vm.get_program_counter(), 0x0206);
}

TEST_F(Chip8Test, ExecuteSKP_V_KeyNotPressed_DoesNotSkip) {
    std::vector<uint8_t> rom = {0x61, 0x05, 0xE1, 0x9E};
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch())); // V1 = 5
    vm.set_keypad_state(5, false);         // Key 5 not pressed
    vm.execute(opcode::decode(vm.fetch())); // SKP V1

    EXPECT_EQ(vm.get_program_counter(), 0x0204);
}

TEST_F(Chip8Test, ExecuteSKNP_V_KeyNotPressed_SkipsNextInstruction) {
    std::vector<uint8_t> rom = {0x61, 0x05, 0xE1, 0xA1};
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch())); // V1 = 5
    vm.set_keypad_state(5, false);         // Key 5 not pressed
    vm.execute(opcode::decode(vm.fetch())); // SKNP V1

    EXPECT_EQ(vm.get_program_counter(), 0x0206);
}

TEST_F(Chip8Test, ExecuteSKNP_V_KeyIsPressed_DoesNotSkip) {
    std::vector<uint8_t> rom = {0x61, 0x05, 0xE1, 0xA1};
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch())); // V1 = 5
    vm.set_keypad_state(5, true);          // Key 5 pressed
    vm.execute(opcode::decode(vm.fetch())); // SKNP V1

    EXPECT_EQ(vm.get_program_counter(), 0x0204);
}

TEST_F(Chip8Test, ExecuteSKP_V_SafelyMasksRegisterOutOfBounds) {
    std::vector<uint8_t> rom = {0x61, 0xFF, 0xE1, 0x9E};
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch())); // V1 = 0xFF
    vm.set_keypad_state(0xF, true);        // Press key 0xF (0xFF & 0x0F)

    // Shouldn't throw out_of_range
    EXPECT_NO_THROW(vm.execute(opcode::decode(vm.fetch())));
    EXPECT_EQ(vm.get_program_counter(), 0x0206);
}

TEST_F(Chip8Test, ExecuteSKNP_V_SafelyMasksRegisterOutOfBounds) {
    std::vector<uint8_t> rom = {0x61, 0xFF, 0xE1, 0xA1};
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch())); // V1 = 0xFF
    vm.set_keypad_state(0xF, false);       // Key 0xF not pressed

    // Shouldn't throw out_of_range
    EXPECT_NO_THROW(vm.execute(opcode::decode(vm.fetch())));
    EXPECT_EQ(vm.get_program_counter(), 0x0206);
}

TEST_F(Chip8Test, ExecuteLD_DT_V_DoesNotModifyVF) {
    std::vector<uint8_t> rom = {
        0x6F, 0xF0,
        0x61, 0x05,
        0xF1, 0x15
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch())); // VF = 0xF0
    vm.execute(opcode::decode(vm.fetch())); // V1 = 0x05
    vm.execute(opcode::decode(vm.fetch())); // DT = V1

    EXPECT_EQ(vm.get_delay_timer(), 0x05);
    EXPECT_EQ(vm.get_register(VF), 0xF0); // VF shouldn't change
}

TEST_F(Chip8Test, ExecuteLD_ST_V_SetsSoundTimer) {
    std::vector<uint8_t> rom = {0x62, 0xFF, 0xF2, 0x18};
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch())); // V2 = 0xFF
    vm.execute(opcode::decode(vm.fetch())); // ST = V2

    EXPECT_EQ(vm.get_sound_timer(), 0xFF);
}

TEST_F(Chip8Test, ExecuteLD_ST_V_DoesNotModifyVF) {
    std::vector<uint8_t> rom = {
        0x6F, 0xF0,
        0x62, 0x00,
        0xF2, 0x18
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch())); // VF = 0xF0
    vm.execute(opcode::decode(vm.fetch())); // V2 = 0x00
    vm.execute(opcode::decode(vm.fetch())); // ST = V2

    EXPECT_EQ(vm.get_sound_timer(), 0x00);
    EXPECT_EQ(vm.get_register(VF), 0xF0); // VF shouldn't change
}

TEST_F(Chip8Test, ExecuteLD_V_DT_ReadsDelayTimer) {
    std::vector<uint8_t> rom = {
        0x61, 0x42, // V1 = 0x42
        0xF1, 0x15, // DT = V1 (0x42)
        0xF2, 0x07  // V2 = DT
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch())); // V1 = 0x42
    vm.execute(opcode::decode(vm.fetch())); // DT = 0x42
    vm.execute(opcode::decode(vm.fetch())); // V2 = DT (0x42)

    EXPECT_EQ(vm.get_register(0x2), 0x42);
}

TEST_F(Chip8Test, ExecuteLD_V_DT_DoesNotModifyVF) {
    std::vector<uint8_t> rom = {
        0x6F, 0xFF, // VF = 0xFF
        0x61, 0x30, // V1 = 0x30
        0xF1, 0x15, // DT = 0x30
        0xF2, 0x07  // V2 = DT
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch())); // VF = 0xFF
    vm.execute(opcode::decode(vm.fetch())); // V1 = 0x30
    vm.execute(opcode::decode(vm.fetch())); // DT = 0x30
    vm.execute(opcode::decode(vm.fetch())); // V2 = DT

    EXPECT_EQ(vm.get_register(0x2), 0x30);
    EXPECT_EQ(vm.get_register(VF), 0xFF); // VF shouldn't change
}

// 8XY1
TEST_F(Chip8Test, ExecuteOR_V_V_DoesNotModifyVF) {
    std::vector<uint8_t> rom = {
        0x61, 0xF0, // V1 = 0xF0
        0x62, 0x0F, // V2 = 0x0F
        0x81, 0x21  // V1 |= V2
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch())); // V1 = 0xF0
    vm.execute(opcode::decode(vm.fetch())); // V2 = 0x0F
    vm.execute(opcode::decode(vm.fetch())); // V1 |= V2

    EXPECT_EQ(vm.get_register(0x1), 0xFF);
    EXPECT_EQ(vm.get_register(VF), 0x00); // VF shouldn't change
}

// [8XY3]
TEST_F(Chip8Test, ExecuteXOR_V_V_DoesNotModifyVF) {
    std::vector<uint8_t> rom = {
        0x61, 0xF0, // V1 = 0xF0
        0x62, 0x0F, // V2 = 0x0F
        0x81, 0x23  // V1 ^= V2
    };
    chip8 vm(rom);

    vm.execute(opcode::decode(vm.fetch())); // V1 = 0xF0
    vm.execute(opcode::decode(vm.fetch())); // V2 = 0x0F
    vm.execute(opcode::decode(vm.fetch())); // V1 ^= V2

    EXPECT_EQ(vm.get_register(0x1), 0xF0 ^ 0x0F);
    EXPECT_EQ(vm.get_register(VF), 0x00); // VF shouldn't change
}