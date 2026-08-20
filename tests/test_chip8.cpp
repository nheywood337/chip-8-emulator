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

// [1nnn] jump to address NNN
TEST_F(Chip8Test, ExecuteJMP_ADDR) {
    std::vector<uint8_t> rom = {0x11, 0x11};
    chip8 vm(rom);

    opcode::Instruction instr = opcode::decode(vm.fetch());
    vm.execute(instr);

    EXPECT_EQ(vm.get_program_counter(), 0x111);
}

// [6xnn] set vX to NN
TEST_F(Chip8Test, ExecuteLD_V_B) {
    std::vector<uint8_t> rom = {0x62, 0x05};
    chip8 vm(rom);

    opcode::Instruction instr = opcode::decode(vm.fetch());
    vm.execute(instr);

    EXPECT_EQ(vm.get_register(instr.x), instr.nn);
}

// [7xnn] add NN to vX
TEST_F(Chip8Test, ExecuteADD_V_B) {
    // 0x62FF -> V2 = 0xFF (255)
    // 0x7205 -> V2 += 0x05 (Rollover to 0x04)
    std::vector<uint8_t> rom = {0x62, 0xFF, 0x72, 0x05};
    chip8 vm(rom);

    // Load V2 with 0xFF
    opcode::Instruction instr = opcode::decode(vm.fetch());
    vm.execute(instr);

    // Fetch and execute 7xnn
    instr = opcode::decode(vm.fetch());
    vm.execute(instr);

    // 0xFF + 0x05 = 0x104 -> rollover to 0x04 in uint8_t
    EXPECT_EQ(vm.get_register(0x2), 0x04);
}

// [00EE] returns from subroutine to address pulled from stack
TEST_F(Chip8Test, ExecuteRET) {
    // 1. 0x2204 -> CALL 0x204 (Subroutine at address 0x204)
    // 2. 0x00EE -> RET (Subroutine body, returns back)
    std::vector<uint8_t> rom = {
        0x22, 0x04, // 0x200: CALL 0x204
        0x00, 0x00, // 0x202: Padding / NOP
        0x00, 0xEE  // 0x204: RET
    };
    chip8 vm(rom);

    // Call subroutine (PC moves from 0x200 -> 0x202 on fetch, pushes 0x202 to stack, sets PC to 0x204)
    opcode::Instruction call_instr = opcode::decode(vm.fetch());
    vm.execute(call_instr);
    EXPECT_EQ(vm.get_program_counter(), 0x204);

    // Fetch and execute RET from 0x204
    opcode::Instruction ret_instr = opcode::decode(vm.fetch());
    vm.execute(ret_instr);

    // PC should be restored to 0x202 (address right after original CALL)
    EXPECT_EQ(vm.get_program_counter(), 0x202);
}

// [2nnn] push return address onto stack and call subroutine at address NNN
TEST_F(Chip8Test, ExecuteCALL_ADDR) {
    std::vector<uint8_t> rom = {0x23, 0x45}; // CALL 0x345
    chip8 vm(rom);

    opcode::Instruction instr = opcode::decode(vm.fetch());
    vm.execute(instr);

    // PC should now point to target address 0x345
    EXPECT_EQ(vm.get_program_counter(), 0x345);
}

// Stack Overflow: A self-referencing CALL pushes return addresses continuously until stack overflows on 17th call
TEST_F(Chip8Test, ExecuteCALL_ADDR_ThrowsOnStackOverflow) {
    // 0x200: CALL 0x200 (Sets PC back to 0x200, creating recursion)
    std::vector<uint8_t> rom = {0x22, 0x00};
    chip8 vm(rom);

    // First 16 recursive calls succeed (filling the 16-element stack)
    for (int i = 0; i < 16; ++i) {
        opcode::Instruction instr = opcode::decode(vm.fetch());
        vm.execute(instr);
    }

    // 17th call exceeds max stack depth (stack_pointer >= 16) and must throw
    opcode::Instruction overflow_instr = opcode::decode(vm.fetch());
    EXPECT_THROW(vm.execute(overflow_instr), chip8_error);
}

// Stack Underflow: Executing RET on a fresh VM with an empty stack should throw
TEST_F(Chip8Test, ExecuteRET_ThrowsOnStackUnderflow) {
    std::vector<uint8_t> rom = {0x00, 0xEE}; // RET
    chip8 vm(rom);

    opcode::Instruction instr = opcode::decode(vm.fetch());
    EXPECT_THROW(vm.execute(instr), chip8_error);
}

// LIFO Ordering: Nested subroutines unwind in reverse order of calls
TEST_F(Chip8Test, NestedSubroutineCallsUnwindInLIFOOrder) {
    // 0x200: CALL 0x206 (Return address: 0x202)
    // 0x202: NOP
    // 0x204: NOP
    // 0x206: CALL 0x20A (Return address: 0x208)
    // 0x208: RET
    // 0x20A: RET
    std::vector<uint8_t> rom = {
        0x22, 0x06, // 0x200: CALL 0x206
        0x00, 0x00, // 0x202
        0x00, 0x00, // 0x204
        0x22, 0x0A, // 0x206: CALL 0x20A
        0x00, 0xEE, // 0x208: RET
        0x00, 0xEE  // 0x20A: RET
    };
    chip8 vm(rom);

    // Call 1: 0x200 -> 0x206 (Stack top: 0x202)
    vm.execute(opcode::decode(vm.fetch()));
    EXPECT_EQ(vm.get_program_counter(), 0x206);

    // Call 2: 0x206 -> 0x20A (Stack top: 0x208)
    vm.execute(opcode::decode(vm.fetch()));
    EXPECT_EQ(vm.get_program_counter(), 0x20A);

    // Ret 1: Pops 0x208 -> PC becomes 0x208
    vm.execute(opcode::decode(vm.fetch()));
    EXPECT_EQ(vm.get_program_counter(), 0x208);

    // Ret 2: Pops 0x202 -> PC becomes 0x202
    vm.execute(opcode::decode(vm.fetch()));
    EXPECT_EQ(vm.get_program_counter(), 0x202);
}
