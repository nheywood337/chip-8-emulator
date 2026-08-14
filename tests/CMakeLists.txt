# fetches GoogleTest source at configure time
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
    GIT_SHALLOW TRUE
)

# recommended by GoogleTest docs for Windows builds
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

add_executable(unit_tests
    test_opcode.cpp
    test_disassembler.cpp
)

target_link_libraries(unit_tests PRIVATE chip8_core GTest::gtest_main)
target_compile_options(unit_tests PRIVATE -Wall -Wextra -Wpedantic)

# registers each TEST() with ctest individually
include(GoogleTest)
gtest_discover_tests(unit_tests)
