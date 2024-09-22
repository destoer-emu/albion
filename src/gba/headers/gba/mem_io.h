#pragma once
#include <albion/lib.h>
namespace gameboyadvance
{


struct KeyCnt
{
    KeyCnt();
    void init();

    u8 read(int idx) const;
    void write(int idx, u8 v);

    u16 key_cnt = 0; // what button do we care about
    bool irq_enable_flag = false; // enable irq
    bool irq_cond = false; // 0 at least one is pressed, 1 all pressed
};


struct SioCnt
{
    SioCnt();
    void init();

    u8 read(int idx) const;
    void write(int idx, u8 v);

    int shift_clock = 0;
    int internal_shift_clock = 0;
    bool si_state = false;
    bool so_during_activity = false;
    bool start = false;
    bool transfer_length = false; // 8 / 32
    bool irq = false;
};


struct WaitCnt
{
    WaitCnt();
    void init();

    void write(int idx, u8 v);
    u8 read(int idx);

    int sram_cnt = 0;
    int wait01 = 0;
    int wait02 = 0;
    int wait11 = 0;
    int wait12 = 0;
    int wait21 = 0;
    int wait22 = 0;
    int term_output = 0;
    bool prefetch = false;
    bool gamepak_flag = false;
};


struct MemIo
{
    MemIo();
    void init();
    u16 keyinput = 0;    
    KeyCnt key_control;
    WaitCnt wait_cnt;
    SioCnt siocnt;
    u8 postflg = 0;
};



}