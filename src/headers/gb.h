#pragma once
#include "cpu.h"
#include "memory.h"
#include "ppu.h"
#include "lib.h"
#include "disass.h"


class GB
{
public:
    // sets up a new emulator state!
    void reset(std::string rom_name);
    void run();
    void key_released(int key);
    void key_pressed(int key);

    Cpu cpu;
    Memory mem;
    Ppu ppu;
    Disass disass;

    bool quit = false;
};