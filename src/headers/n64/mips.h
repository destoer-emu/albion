#pragma once
namespace nintendo64
{
    
static constexpr int R0 = 0;
static constexpr int AT = 1;
static constexpr int V0 = 2;
static constexpr int V1 = 3;
static constexpr int A0 = 4;
static constexpr int A1 = 5;
static constexpr int A2 = 6;
static constexpr int A3 = 7;
static constexpr int T0 = 8;
static constexpr int T1 = 9;
static constexpr int T2 = 10;
static constexpr int T3 = 11;
static constexpr int T4 = 12;
static constexpr int T5 = 13;
static constexpr int T6 = 14;
static constexpr int T7 = 15;
static constexpr int S0 = 16;
static constexpr int S1 = 17;
static constexpr int S2 = 18;
static constexpr int S3 = 19;
static constexpr int S4 = 20;
static constexpr int S5 = 21;
static constexpr int S6 = 22;
static constexpr int S7 = 23;
static constexpr int T8 = 24;
static constexpr int T9 = 25;
static constexpr int K0 = 26;
static constexpr int K1 = 27;
static constexpr int GP = 28;
static constexpr int SP = 29;
static constexpr int FP = 30; // also reffered to as fp (frame pointer)
static constexpr int RA = 31;


// cop0 regs
static constexpr int RANDOM = 1;
static constexpr int STATUS = 12;
static constexpr int PRID = 15;
static constexpr int CONFIG = 16;

// constants for disassembling mips
extern const char *r[32];

}