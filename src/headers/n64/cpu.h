#pragma once
#include <n64/forward_def.h>
#include <destoer-emu/lib.h>
#include <n64/mips.h>

namespace nintendo64
{


struct Cpu
{
    u64 regs[32];
    u64 pc;

    u64 cp0_regs[32];
};


void step(N64 &n64);
void reset_cpu(Cpu &cpu);

}