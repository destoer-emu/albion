#pragma once
#include <gb/forward_def.h>
#include <destoer-emu/lib.h>
#include <destoer-emu/debug.h>
#include <destoer-emu/scheduler.h>

namespace gameboy
{

// just easy to put here
enum class gameboy_event
{
    oam_dma_end,
    c1_period_elapse,
    c2_period_elapse,
    c3_period_elapse,
    c4_period_elapse,
    sample_push,
    internal_timer,
    timer_reload,
    ppu,
    serial
};

constexpr size_t EVENT_SIZE = 10;

class GameboyScheduler : public Scheduler<EVENT_SIZE,gameboy_event>
{
public:
    GameboyScheduler(GB &gb);

    bool is_double() const;
    void skip_to_event();

protected:
    void service_event(const EventNode<gameboy_event> & node) override;
private:
    
    Cpu &cpu;
    Ppu &ppu;
    Apu &apu;
    Memory &mem;
};

}