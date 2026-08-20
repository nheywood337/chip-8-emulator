cmake_minimum_required(VERSION 3.16)
project(chip8_emulator CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define warnings once for reuse across targets
set(PROJECT_WARNING_FLAGS -Wall -Wextra -Wpedantic)

# Core Library
add_library(chip8_core
    src/chip8.cpp
    src/rom_loader.cpp
    src/disassembler.cpp
    src/opcode.cpp
)

target_include_directories(chip8_core PUBLIC include)
target_compile_options(chip8_core PRIVATE ${PROJECT_WARNING_FLAGS})

# Main Executable
add_executable(chip8 src/main.cpp)
target_link_libraries(chip8 PRIVATE chip8_core)
target_compile_options(chip8 PRIVATE ${PROJECT_WARNING_FLAGS})

# Testing Setup
include(CTest) # Automatically defines BUILD_TESTING option (defaults to ON)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()