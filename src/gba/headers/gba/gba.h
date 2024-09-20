#pragma once
#include <gba/cpu.h>
#include <gba/memory.h>
#include <gba/disass.h>
#include <gba/display.h>
#include <gba/apu.h>
#include <gba/scheduler.h>
#include <gba/debug.h>
#include <albion/debug.h>
#include <albion/input.h>

namespace gameboyadvance
{

enum class button 
{
    a = 0,b=1,select=2,start=3,
    right=4,left=5,up=6,down=7,
    r=8,l=9
};

struct GBA
{
    GBA();

    void reset(std::string filename);
    void run();
    
    
    void button_event(button b, bool down); //actual hanlder
    void handle_input(Controller& controller);

#ifdef DEBUG
    void change_breakpoint_enable(bool enabled);
#endif

    // NOTE: see n64 core for better example of how to structure this
    // this is not bad enough to be worthwhile changing here
    Cpu cpu{*this};
    Mem mem{*this};
    Disass disass{*this};
    Display disp{*this};
    Apu apu{*this};
    GBAScheduler scheduler{*this};
    GBADebug debug{*this};

   bool quit = false;

   bool throttle_emu = true;
};

}