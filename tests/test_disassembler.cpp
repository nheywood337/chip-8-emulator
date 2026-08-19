#include <gtest/gtest.h>

#include "disassembler.h"
#include "opcode.h"
#include "chip8_specs.h"

struct MnemonicCase {
    uint16_t raw_opcode;
    std::string expected;
};

class MnemonicParamTest : public ::testing::TestWithParam<MnemonicCase> {};

TEST_P(MnemonicParamTest, ProducesExpectedMnemonic) {
    auto instruction = opcode::decode(GetParam().raw_opcode);
    EXPECT_EQ(disassembler::mnemonic(instruction), GetParam().expected);
}

INSTANTIATE_TEST_SUITE_P(
    AllOpcodes,
    MnemonicParamTest,
    ::testing::Values(
        // System / Jump
        MnemonicCase{0x00E0, "CLS"},
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
        MnemonicCase{0xF965, "LD V9, [I]"},

        // Invalid codes
        MnemonicCase{0x5121, "???"},
        MnemonicCase{0x9121, "???"},
        MnemonicCase{0xE01E, "???"},
        MnemonicCase{0x8018, "???"}
    )
);

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
