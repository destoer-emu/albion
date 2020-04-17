#pragma once
#include <destoer-emu/lib.h>
namespace gameboyadvance
{


struct KeyCnt
{
    KeyCnt();
    void init();

    uint8_t read(int idx) const;
    void write(int idx, uint8_t v);

    uint16_t key_cnt; // what button do we care about
    bool irq_enable_flag; // enable irq
    bool irq_cond; // 0 at least one is pressed, 1 all pressed
};






struct MemIo
{
    MemIo();
    void init();

    // input
    uint16_t keyinput;
    KeyCnt key_control;
};

}