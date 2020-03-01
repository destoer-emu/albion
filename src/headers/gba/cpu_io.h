#pragma once
#include <destoer-emu/lib.h>
#include <gba/arm.h>

namespace gameboyadvance
{

struct HaltCnt
{
    HaltCnt();
    void init();

    // write only
    void write(uint8_t v);


    enum class power_state
    {
        halt,
        stop,
        normal
    };

    power_state state;
};


// cpu io registers
struct CpuIo
{
    CpuIo();
    void init();


    // interrupt master enable
    bool ime;
    uint16_t interrupt_enable;
    uint16_t interrupt_flag;
    HaltCnt halt_cnt;
};

} 