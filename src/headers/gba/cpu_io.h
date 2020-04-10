#pragma once
#include <destoer-emu/lib.h>
#include <gba/arm.h>
#include <gba/interrupt.h>

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


struct TimerCounter
{
    TimerCounter();

    void init();

    uint8_t read_counter(int idx) const;

    // actually writes the reload but is at the same addr
    void write_counter(int idx, uint8_t v);

    uint8_t read_control() const;
    void write_control(uint8_t v);

    // counter
    uint16_t reload;
    uint16_t counter;


    int cycle_count;

    // control
    int scale;
    bool count_up;
    bool irq;
    bool enable;


    static constexpr int cycle_limit[4] = {1,64,256,1024};
    static constexpr interrupt timer_interrupt[4] = 
    {
        interrupt::timer0,
        interrupt::timer1,
        interrupt::timer2,
        interrupt::timer3
    };
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


    std::array<TimerCounter,4> timers;
};

} 