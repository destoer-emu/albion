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

static constexpr int COUNT = 9;

static constexpr int COMPARE = 11;
static constexpr int STATUS = 12;

static constexpr int CAUSE = 13;

static constexpr int PRID = 15;
static constexpr int CONFIG = 16;

static constexpr int TAGLO = 28;
static constexpr int TAGHI = 29;

// constants for disassembling mips
extern const char *reg_names[32];
extern const char *cp0_names[32];

// sign extend on a 8 bit aligned quantity
template<typename OUT,typename IN>
inline OUT sign_extend_mips(IN x)
{
    using signed_type_in = typename std::make_signed<IN>::type;
    const auto v = static_cast<signed_type_in>(x);

    using signed_type_out = typename std::make_signed<OUT>::type;
    return static_cast<OUT>(static_cast<signed_type_out>(v));
}

inline u32 get_rt(u32 opcode)
{
    return (opcode >> 16) & 0x1f;
}

inline u32 get_rd(u32 opcode)
{
    return (opcode >> 11) & 0x1f;
}

inline u32 get_rs(u32 opcode)
{
    return (opcode >> 21) & 0x1f;
}

inline u32 get_shamt(u32 opcode)
{
    return (opcode >> 6) & 0x1f;
}

inline u64 get_target(u32 opcode, u64 pc)
{
    return ((opcode & 0x3FFFFFF) << 2) | (pc & 0xfffffffff0000000);
}

inline u64 compute_branch_addr(u64 pc, u16 imm)
{
    return pc + (sign_extend_mips<s32,s16>(imm) << 2);
}

}