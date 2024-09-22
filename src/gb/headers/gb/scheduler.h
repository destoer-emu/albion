#pragma once
#include <gb/forward_def.h>
#include <albion/lib.h>
#include <albion/debug.h>
#include <albion/scheduler.h>

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
    serial,
    cycle_frame,
};

constexpr size_t EVENT_SIZE = 11;

struct GameboyScheduler final : public Scheduler<EVENT_SIZE,gameboy_event>
{
    GameboyScheduler(GB &gb);

    bool is_double() const;
    void skip_to_event();

    Cpu &cpu;
    Ppu &ppu;
    Apu &apu;
    Memory &mem;

protected:
    void service_event(const EventNode<gameboy_event> & node) override;
};

}