#include <gtest/gtest.h>

#include "opcode.h"

struct OpcodeCase {
    uint16_t raw_opcode;
    opcode::Opcode expected;
};

class OpcodeParamTest : public ::testing::TestWithParam<OpcodeCase> {};

TEST_P(OpcodeParamTest, ProducesExpectedOpcode) {
    auto instruction = opcode::decode(GetParam().raw_opcode);
    EXPECT_EQ(instruction.opcode, GetParam().expected);
}

INSTANTIATE_TEST_SUITE_P(
    AllOpcodeEnums,
    OpcodeParamTest,
    ::testing::Values(
        // System / Jump
        OpcodeCase{0x00E0, opcode::Opcode::CLS},
        OpcodeCase{0x00EE, opcode::Opcode::RET},
        OpcodeCase{0x0123, opcode::Opcode::SYS_ADDR},
        OpcodeCase{0x1234, opcode::Opcode::JP_ADDR},
        OpcodeCase{0x2ABC, opcode::Opcode::CALL_ADDR},

        // Skip / Load / Add Immediates
        OpcodeCase{0x31AB, opcode::Opcode::SE_V_B},
        OpcodeCase{0x42CD, opcode::Opcode::SNE_V_B},
        OpcodeCase{0x5340, opcode::Opcode::SE_V_V},
        OpcodeCase{0x6567, opcode::Opcode::LD_V_B},
        OpcodeCase{0x7889, opcode::Opcode::ADD_V_B},

        // 0x8 ALU Operations
        OpcodeCase{0x8010, opcode::Opcode::LD_V_V},
        OpcodeCase{0x8231, opcode::Opcode::OR_V_V},
        OpcodeCase{0x8452, opcode::Opcode::AND_V_V},
        OpcodeCase{0x8673, opcode::Opcode::XOR_V_V},
        OpcodeCase{0x8894, opcode::Opcode::ADD_V_V},
        OpcodeCase{0x8AB5, opcode::Opcode::SUB_V_V},
        OpcodeCase{0x8CD6, opcode::Opcode::SHR_V},
        OpcodeCase{0x8EF7, opcode::Opcode::SUBN_V_V},
        OpcodeCase{0x801E, opcode::Opcode::SHL_V},

        // Skip Registers / Jump V0 / Random / Draw
        OpcodeCase{0x9120, opcode::Opcode::SNE_V_V},
        OpcodeCase{0xA123, opcode::Opcode::LD_I_ADDR},
        OpcodeCase{0xB456, opcode::Opcode::JP_V_ADDR},
        OpcodeCase{0xC122, opcode::Opcode::RND_V_B},
        OpcodeCase{0xD125, opcode::Opcode::DRW_V_V},

        // Keyboard Skips
        OpcodeCase{0xE19E, opcode::Opcode::SKP_V},
        OpcodeCase{0xE2A1, opcode::Opcode::SKNP_V},

        // 0xF Timers / Memory
        OpcodeCase{0xF107, opcode::Opcode::LD_V_DT},
        OpcodeCase{0xF20A, opcode::Opcode::LD_V_K},
        OpcodeCase{0xF315, opcode::Opcode::LD_DT_V},
        OpcodeCase{0xF418, opcode::Opcode::LD_ST_V},
        OpcodeCase{0xF51E, opcode::Opcode::ADD_I_V},
        OpcodeCase{0xF629, opcode::Opcode::LD_F_V},
        OpcodeCase{0xF733, opcode::Opcode::LD_B_V},
        OpcodeCase{0xF855, opcode::Opcode::LD_I_V},
        OpcodeCase{0xF965, opcode::Opcode::LD_V_I},

        // Invalid codes
        OpcodeCase{0x5121, opcode::Opcode::INVALID},
        OpcodeCase{0x9121, opcode::Opcode::INVALID},
        OpcodeCase{0xE01E, opcode::Opcode::INVALID},
        OpcodeCase{0x8018, opcode::Opcode::INVALID}
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