cmake_minimum_required(VERSION 3.16)
project(chip8_emulator CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PROJECT_WARNING_FLAGS -Wall -Wextra -Wpedantic)

include(FetchContent)

set(SDL_SHARED          OFF CACHE BOOL "" FORCE)
set(SDL_STATIC          ON  CACHE BOOL "" FORCE)
set(SDL_TEST            OFF CACHE BOOL "" FORCE)
set(SDL_AUDIO           ON  CACHE BOOL "" FORCE)
set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.30.9
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(SDL2)

add_library(chip8_core
    src/chip8.cpp
    src/rom_loader.cpp
    src/disassembler.cpp
    src/opcode.cpp
)

target_include_directories(chip8_core PUBLIC include)
target_compile_options(chip8_core PRIVATE ${PROJECT_WARNING_FLAGS})

add_executable(chip8
    src/main.cpp
    src/platform.cpp
)
target_link_libraries(chip8 PRIVATE chip8_core SDL2::SDL2-static)
target_compile_options(chip8 PRIVATE ${PROJECT_WARNING_FLAGS})

# Testing Setup
include(CTest) # Automatically defines BUILD_TESTING option (defaults to ON)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
