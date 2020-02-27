#pragma once
#include <destoer-emu/lib.h>

namespace gameboyadvance
{

// cpu io registers
struct CpuIo
{
    CpuIo();
    void init();


    // interrupt master enable
    bool ime;
    uint16_t interrupt_enable;
    uint16_t interrupt_flag;
};

}