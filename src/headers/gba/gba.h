#pragma once
#include <gba/cpu.h>
#include <gba/memory.h>
#include <gba/disass.h>
#include <gba/display.h>
#include <gba/apu.h>
#include <destoer-emu/debug.h>

namespace gameboyadvance
{

enum class button 
{
    a = 0,b=1,select=2,start=3,
    right=4,left=5,up=6,down=7,
    r=8,l=9
};

class GBA
{
public:
    GBA();

    void reset(std::string filename);
    void run();
    
    
    void button_event(button b, bool down); //actual hanlder
    void key_input(int key, bool pressed); // takes the input and passes it on

#ifdef DEBUG
    void change_breakpoint_enable(bool enabled);
#endif

    Cpu cpu{*this};
    Mem mem{*this};
    Disass disass{*this};
    Display disp{*this};
    Apu apu{*this};
    Debug debug;

   std::atomic_bool quit = false;

   std::atomic_bool throttle_emu = false;
};

}