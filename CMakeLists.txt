cmake_minimum_required(VERSION 3.16)
project(chip8_emulator CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(chip8
    src/main.cpp
    src/chip8.cpp
    src/rom_loader.cpp
    src/disassembler.cpp
)

target_include_directories(chip8 PRIVATE include)

target_compile_options(chip8 PRIVATE -Wall -Wextra -Wpedantic)
