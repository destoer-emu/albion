#pragma once
#include <n64/cpu.h>
#include <n64/mem.h>
#include <n64/rdp.h>
#include <n64/disass.h>
#include <n64/debug.h>
#include <destoer-emu/lib.h>

namespace nintendo64
{

struct N64
{
    Cpu cpu;
    Mem mem;
    Rdp rdp;
    N64Debug debug{*this};

    u32 cycles = 0;
    bool quit = false;
    bool size_change = false;
};

static constexpr u32 N64_CLOCK_CYCLES = 93 * 1024 * 1024;

void reset(N64 &n64, const std::string &filename);
void run(N64 &n64);
}