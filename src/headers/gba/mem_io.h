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


struct SioCnt
{
    SioCnt();
    void init();

    uint8_t read(int idx) const;
    void write(int idx, uint8_t v);

    int shift_clock;
    int internal_shift_clock;
    bool si_state;
    bool so_during_activity;
    bool start;
    bool transfer_length; // 8 / 32
    bool irq;
};



struct MemIo
{
    MemIo();
    void init();
    uint16_t keyinput;    
    KeyCnt key_control;
    SioCnt siocnt;
    uint8_t postflg;
};

}