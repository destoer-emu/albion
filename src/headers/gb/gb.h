#pragma once
#include "cpu.h"
#include "memory.h"
#include "apu.h"
#include "ppu.h"
#include <destoer-emu/lib.h>
#include "disass.h"
#include <destoer-emu/debug.h>

namespace gameboy
{
class GB
{
public:
    // sets up a new emulator state!
    GB();
    void reset(std::string rom_name, bool with_rom=true);
    void run();

    void key_input(int key,bool pressed);
    void key_released(int key);
    void key_pressed(int key);

    void save_state(std::string filename);
    void load_state(std::string filename);

    Cpu cpu;
    Memory mem;
    Ppu ppu;
    Apu apu;
    Disass disass;
    Debug debug;

    bool quit = false;
    bool throttle_emu = true;
};

}