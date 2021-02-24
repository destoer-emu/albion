#pragma once
#include <gba/forward_def.h>
#include <destoer-emu/lib.h>
#include <destoer-emu/debug.h>
#include <destoer-emu/scheduler.h>

namespace gameboyadvance
{

// just easy to put here
enum class gba_event
{
    sample_push,
    c1_period_elapse,
    c2_period_elapse,
    c3_period_elapse,
    c4_period_elapse,
    psg_sequencer,
    timer0,
    timer1,
    timer2,
    timer3,
    display
};

constexpr size_t EVENT_SIZE = 11;

class GBAScheduler : public Scheduler<EVENT_SIZE,gba_event>
{
public:
    GBAScheduler(GBA &gba);


    void skip_to_event();

protected:
    void service_event(const EventNode<gba_event> & node) override;
private:
    
    Cpu &cpu;
    Display &disp;
    Apu &apu;
    Mem &mem;
};

}