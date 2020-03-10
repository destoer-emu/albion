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

enum class button
{
    a = 4,
    b = 5,
    start = 7,
    select = 6,
    right = 0,
    left = 1,
    up = 2,
    down = 3 
};

class GB
{
public:
    // sets up a new emulator state!
    GB();
    void reset(std::string rom_name, bool with_rom=true, bool use_bios = false);
    void run();

    void key_input(int key,bool pressed);
    void key_released(button b);
    void key_pressed(button b);

    void save_state(std::string filename);
    void load_state(std::string filename);

    Cpu cpu;
    Memory mem;
    Ppu ppu;
    Apu apu;
    Disass disass;
    Debug debug;

    std::atomic_bool quit = false;
    bool throttle_emu = true;
};

}