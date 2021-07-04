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


using INSTR_FUNC = void (*)(N64 &n64, u32 opcode);

void step(N64 &n64);
void reset_cpu(Cpu &cpu);


void cycle_tick(N64 &n64, u32 cycles);

void write_cp0(N64 &n64, u64 v, u32 reg);

// instruction handlers
void instr_unknown(N64 &n64, u32 opcode);
void instr_unknown_cop0(N64 &n64, u32 opcode);

void instr_cop0(N64 &n64, u32 opcode);
void instr_mtc0(N64 &n64, u32 opcode); 


}