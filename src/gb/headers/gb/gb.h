#pragma once
#include <gb/memory.h>
#include <gb/ppu.h>
#include <gb/apu.h>
#include <gb/cpu.h>
#include <gb/scheduler.h>
#include <gb/disass.h>
#include <albion/lib.h>
#include <albion/input.h>
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

struct GB
{
    GB();
    void reset(std::string rom_name, bool with_rom=true, bool use_bios = false);
    void run();


    void handle_input(Controller& controller);
    void key_input(button b, b32 down);
    void key_released(button b);
    void key_pressed(button b);

    void save_state(std::string filename);
    void load_state(std::string filename);

#ifdef DEBUG
    void change_breakpoint_enable(bool enabled);
#endif

    // NOTE: see n64 core for better example of how to structure this
    // the effort required to fix this isn't worthwhile however
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