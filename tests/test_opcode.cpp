#include <gtest/gtest.h>

#include "opcode.h"

// test each distinct nibble
TEST(OpcodeDecodeTest, DistinctNibbleOpcodeDecodesAllFieldsCorrectly) {
    opcode::Instruction instruction = opcode::decode(0x1234);

    EXPECT_EQ(instruction.f, 0x1);
    EXPECT_EQ(instruction.x, 0x2);
    EXPECT_EQ(instruction.y, 0x3);
    EXPECT_EQ(instruction.n, 0x4);
    EXPECT_EQ(instruction.nn, 0x34);
    EXPECT_EQ(instruction.nnn, 0x234);
}
