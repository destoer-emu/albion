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
    u64 pc_next;

    u64 lo;
    u64 hi;

    u64 cp0_regs[32];


    // cp0 status
    bool ie;
    bool exl;
    bool erl;
    u32 ksu;
    bool ux;
    bool sx;
    bool kx;
    u8 im;
    u32 ds;
    bool re;
    bool fr;
    bool rp;

    // other control bits are unused
    bool cu1;
};


struct Opcode
{
    u32 op;

    u32 rs;
    u32 rd;
    u32 rt;
    u16 imm;
};


using INSTR_FUNC = void (*)(N64 &n64, const Opcode &opcode);

void step(N64 &n64);
void reset_cpu(Cpu &cpu);

void skip_instr(Cpu &cpu);
void write_pc(N64 &n64, u64 pc);

void cycle_tick(N64 &n64, u32 cycles);

void write_cp0(N64 &n64, u64 v, u32 reg);


}