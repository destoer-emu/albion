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
    void reset(std::string filename);
    void run();
    
    
    //void handle_input();
    void button_event(button b, bool down); //actual hanlder
    void key_input(int key, bool pressed); // takes the input and passes it on

    Cpu cpu;
    Mem mem;
    Disass disass;
    Display disp;
    Debug debug;
    Apu apu;

   std::atomic_bool quit = false;
};

}