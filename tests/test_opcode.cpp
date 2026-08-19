#include <gtest/gtest.h>

#include "opcode.h"

struct MnemonicCase {
    uint16_t raw_opcode;
    std::string expected;
};

class MnemonicParamTest : public ::testing::TestWithParam<MnemonicCase> {};

TEST_P(MnemonicParamTest, ProducesExpectedMnemonic) {
    auto instruction = opcode::decode(GetParam().raw_opcode);
    EXPECT_EQ(opcode::decode, GetParam().expected);
}

// TODO FIX
INSTANTIATE_TEST_SUITE_P(
    AllOpcodes,
    MnemonicParamTest,
    ::testing::Values(
        // System / Jump
        MnemonicCase{0x00E0, opcode::Opcode::CLS},
        MnemonicCase{0x00EE, "RET"},
        MnemonicCase{0x0123, "SYS 123"},
        MnemonicCase{0x1234, "JP 234"},
        MnemonicCase{0x2ABC, "CALL ABC"},

        // Skip / Load / Add Immediates
        MnemonicCase{0x31AB, "SE V1, AB"},
        MnemonicCase{0x42CD, "SNE V2, CD"},
        MnemonicCase{0x5340, "SE V3, V4"},
        MnemonicCase{0x6567, "LD V5, 67"},
        MnemonicCase{0x7889, "ADD V8, 89"},
 
        // 0x8 ALU Operations
        MnemonicCase{0x8010, "LD V0, V1"},
        MnemonicCase{0x8231, "OR V2, V3"},
        MnemonicCase{0x8452, "AND V4, V5"},
        MnemonicCase{0x8673, "XOR V6, V7"},
        MnemonicCase{0x8894, "ADD V8, V9"},
        MnemonicCase{0x8AB5, "SUB VA, VB"},
        MnemonicCase{0x8CD6, "SHR VC, VD"},
        MnemonicCase{0x8EF7, "SUBN VE, VF"},
        MnemonicCase{0x801E, "SHL V0, V1"},

        // Skip Registers / Jump V0 / Random / Draw
        MnemonicCase{0x9120, "SNE V1, V2"},
        MnemonicCase{0xA123, "LD I, 123"},
        MnemonicCase{0xB456, "JP V0, 456"},
        MnemonicCase{0xC122, "RND V1, 22"},
        MnemonicCase{0xD125, "DRW V1, V2, 5"},

        // Keyboard Skips
        MnemonicCase{0xE19E, "SKP V1"},
        MnemonicCase{0xE2A1, "SKNP V2"},

        // 0xF Timers / Memory
        MnemonicCase{0xF107, "LD V1, DT"},
        MnemonicCase{0xF20A, "LD V2, K"},
        MnemonicCase{0xF315, "LD DT, V3"},
        MnemonicCase{0xF418, "LD ST, V4"},
        MnemonicCase{0xF51E, "ADD I, V5"},
        MnemonicCase{0xF629, "LD F, V6"},
        MnemonicCase{0xF733, "LD B, V7"},
        MnemonicCase{0xF855, "LD [I], V8"},
        MnemonicCase{0xF965, "LD V9, [I]"}
    )
);

// test each distinct nibble
TEST(OpcodeDecodeTest, DistinctNibbleOpcodeDecodesAllFieldsCorrectly) {
    opcode::Instruction instr = opcode::decode(0x1234);

    EXPECT_EQ(instr.f, 0x1);
    EXPECT_EQ(instr.x, 0x2);
    EXPECT_EQ(instr.y, 0x3);
    EXPECT_EQ(instr.n, 0x4);
    EXPECT_EQ(instr.nn, 0x34);
    EXPECT_EQ(instr.nnn, 0x234);
    EXPECT_EQ(instr.opcode, opcode::Opcode::JP_ADDR);
}

// test valid opcode
TEST(OpcodeDecodeTest, ValidCLS) {
    opcode::Instruction instr = opcode::decode(0x00E0);

    EXPECT_EQ(instr.f, 0x0);
    EXPECT_EQ(instr.x, 0x0);
    EXPECT_EQ(instr.y, 0xE);
    EXPECT_EQ(instr.n, 0x0);
    EXPECT_EQ(instr.nn, 0xE0);
    EXPECT_EQ(instr.nnn, 0x0E0);
    EXPECT_EQ(instr.opcode, opcode::Opcode::CLS);
}
