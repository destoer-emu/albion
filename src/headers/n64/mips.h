#pragma once
namespace nintendo64
{
    

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