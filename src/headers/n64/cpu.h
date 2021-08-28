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
    u64 pc_old;

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


using INSTR_FUNC = void (*)(N64 &n64, u32 opcode);

void step(N64 &n64);
void reset_cpu(Cpu &cpu);


void cycle_tick(N64 &n64, u32 cycles);

void write_cp0(Cpu &cpu, u64 v, u32 reg);

// instruction handlers
void instr_unknown(N64 &n64, u32 opcode);
void instr_unknown_cop0(N64 &n64, u32 opcode);
void instr_unknown_r(N64 &n64, u32 opcode);

void instr_lui(N64 &n64, u32 opcode);
void instr_addiu(N64 &n64, u32 opcode);
void instr_addi(N64 &n64, u32 opcode);
void instr_ori(N64 &n64, u32 opcode);
void instr_jal(N64 &n64, u32 opcode);
void instr_slti(N64 &n64, u32 opcode);


void instr_lw(N64 &n64, u32 opcode);
void instr_sw(N64 &n64, u32 opcode);

void instr_bne(N64 &n64, u32 opcode);


void instr_sll(N64 &n64, u32 opcode);
void instr_or(N64 &n64, u32 opcode);

void instr_cop0(N64 &n64, u32 opcode);
void instr_mtc0(N64 &n64, u32 opcode);
void instr_r_fmt(N64 &n64, u32 opcode);


}