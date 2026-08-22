#pragma once

#include <cstdint>
#include <cstddef>


constexpr size_t MEMORY_SIZE = 4096;
constexpr uint16_t START_ADDRESS_OFFSET = 0x200;
constexpr size_t DISPLAY_WIDTH = 64;
constexpr size_t DISPLAY_HEIGHT = 32;
static constexpr uint8_t VF = 0xF;

// Font sprites for Fx29 (16 glyphs, 5 bytes each)
constexpr uint16_t FONT_START_ADDRESS = 0x000;
constexpr size_t FONT_SPRITE_SIZE = 5;
constexpr size_t FONT_SET_SIZE = 16 * FONT_SPRITE_SIZE;

