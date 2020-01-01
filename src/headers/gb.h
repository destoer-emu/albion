#pragma once
#include "cpu.h"
#include "memory.h"
#include "apu.h"
#include "ppu.h"
#include "lib.h"
#include "disass.h"
#include "debug.h"


class GB
{
public:
    // sets up a new emulator state!
    void reset(std::string rom_name, bool with_rom=true);
    void run();
    void key_released(int key);
    void key_pressed(int key);

    Cpu cpu;
    Memory mem;
    Ppu ppu;
    Apu apu;
    Disass disass;
    Debug debug;

    bool quit = false;
};