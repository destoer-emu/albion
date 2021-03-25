#pragma once
#include <gb/memory.h>
#include <gb/ppu.h>
#include <gb/apu.h>
#include <gb/cpu.h>
#include <gb/scheduler.h>
#include <gb/disass.h>
#include <destoer-emu/lib.h>
#include <gb/debug.h>

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
    GB();
    void reset(std::string rom_name, bool with_rom=true, bool use_bios = false);
    void run();

    void key_input(int key,bool pressed);
    void key_released(button b);
    void key_pressed(button b);

    void save_state(std::string filename);
    void load_state(std::string filename);

#ifdef DEBUG
    void change_breakpoint_enable(bool enabled);
#endif

    Cpu cpu{*this};
    Memory mem{*this};
    Ppu ppu{*this};
    Apu apu{*this};
    Disass disass{*this};
    GameboyScheduler scheduler{*this};
    GBDebug debug{*this};


    std::atomic_bool quit = false;
    bool throttle_emu = true;
};

}