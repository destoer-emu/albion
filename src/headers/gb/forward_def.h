#pragma once
#include <destoer-emu/lib.h>

namespace gameboy
{

class Memory;
class Cpu;
class Ppu;
class Disass;
class Apu;
class Scheduler;
class GB;

// just easy to put here
enum class event_type
{
    oam_dma_end,
    c1_period_elapse,
    c2_period_elapse,
    c3_period_elapse,
    c4_period_elapse
};

constexpr size_t EVENT_SIZE = 5;


}