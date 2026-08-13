# CHIP-8-Emulator
A from-scratch CHIP-8 interpreter in C++

## To build the program
```bash
cmake -S . -B build
cmake --build build
```

## To run the program

The emulator supports two modes: run (`-r`) and disassemble (`-d`).

Run a ROM (WORK IN PROGRESS):
```bash
./build/chip8 -r roms/<ROM_TO_LOAD>
```

Disassemble a ROM into a listing:
```bash
./build/chip8 -d roms/<ROM_TO_LOAD>
```
