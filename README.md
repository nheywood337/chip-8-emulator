# CHIP-8-Emulator
A from-scratch CHIP-8 interpreter in C++

## To build the program

CMake fetches and builds SDL2 for you, but SDL needs the X11 headers to find a
video driver:
```bash
sudo apt install libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libgl1-mesa-dev
```

```bash
cmake -S . -B build
cmake --build build
```

## To run the program

The emulator supports two modes: run (`-r`) and disassemble (`-d`).

Run a ROM:
```bash
./build/chip8 -r roms/<ROM_TO_LOAD>
```

Disassemble a ROM into a listing:
```bash
./build/chip8 -d roms/<ROM_TO_LOAD>
```

## Sound

A 440 Hz square wave plays while the CHIP-8 sound timer is running. If the
machine has no audio device the emulator says so on stderr and runs silently.
`roms/7-beep.ch8` exercises it.

## Controls

The keypad sits on the left of the keyboard. Esc quits.

```
1 2 3 4  ->  1 2 3 C
Q W E R  ->  4 5 6 D
A S D F  ->  7 8 9 E
Z X C V  ->  A 0 B F
```
