#pragma once
#include <n64/cpu.h>
#include <n64/mem.h>
#include <n64/rdp.h>
#include <n64/debug.h>
#include <n64/scheduler.h>
#include <albion/lib.h>
#include <beyond_all_repair.h>
#include <albion/input.h>

namespace nintendo64
{

struct N64
{
    Cpu cpu;
    Mem mem;
    Rdp rdp;
    N64Debug debug{*this};
    N64Scheduler scheduler{*this};
    beyond_all_repair::Program program;

    bool quit = false;
    bool size_change = false;
    b32 debug_enabled = false;
};

static constexpr u32 N64_CLOCK_CYCLES = 93 * 1024 * 1024;
static constexpr u32 N64_CLOCK_CYCLES_FRAME =  N64_CLOCK_CYCLES / 60;


void reset(N64 &n64, const std::string &filename);
void run(N64 &n64);

std::string disass_n64(N64& n64, Opcode opcode, u64 addr);
void handle_input(N64& n64, Controller& controller);
const char* reg_name(u32 idx);
}