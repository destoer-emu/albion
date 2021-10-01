#pragma once
#include <n64/cpu.h>
#include <n64/mem.h>
#include <n64/disass.h>
#include <n64/debug.h>
#include <n64/mips_lut.h>
#include <destoer-emu/lib.h>

namespace nintendo64
{

struct N64
{
    Cpu cpu;
    Mem mem;
    N64Debug debug{*this};

    bool quit = false;
};

void reset(N64 &n64, const std::string &filename);
void run(N64 &n64);

}