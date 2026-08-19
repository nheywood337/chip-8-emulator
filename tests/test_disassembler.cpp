#include <gtest/gtest.h>

#include "disassembler.h"
#include "opcode.h"
#include "chip8_specs.h"

// case (0x0) branches on nn for CLS/RET/SYS
TEST(MnemonicTest, ZeroZeroE0IsCls) {
    auto result = disassembler::mnemonic(opcode::decode(0x00E0));

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "CLS");
}

TEST(MnemonicTest, ZeroZeroEEIsRet) {
    auto result = disassembler::mnemonic(opcode::decode(0x00EE));

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "RET");
}

TEST(MnemonicTest, ZeroNnnFallsThroughToSys) {
    auto result = disassembler::mnemonic(opcode::decode(0x0123));

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "SYS 123");
}

// nested switches default to nullopt for unhandled
TEST(MnemonicTest, DefinedEightXyVariantResolves) {
    // f=8, x=0, y=1, n=4 -> "ADD Vx, Vy"
    auto result = disassembler::mnemonic(opcode::decode(0x8014));

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "ADD V0, V1");
}

TEST(MnemonicTest, UndefinedEightXyVariantReturnsNullopt) {
    // n=8 isn't a defined 8xy_ variant
    auto result = disassembler::mnemonic(opcode::decode(0x8018));

    EXPECT_FALSE(result.has_value());
}

TEST(DisassembleTest, OddLengthRomIsMalformed) {
    std::vector<uint8_t> odd_length_rom = {0x00, 0xE0, 0x12};

    auto result = disassembler::disassemble(odd_length_rom);

    EXPECT_FALSE(result.has_value());
}

TEST(DisassembleTest, TwoInstructionRomProducesTwoCorrectlyAddressedLines) {
    // 0x00E0 = CLS, 0x1234 = JP 234
    std::vector<uint8_t> rom_bytes = {0x00, 0xE0, 0x12, 0x34};

    auto result = disassembler::disassemble(rom_bytes);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 2u);

    EXPECT_EQ((*result)[0], "0200 | 00e0 | CLS");
    EXPECT_EQ((*result)[1], "0202 | 1234 | JP 234");
}
