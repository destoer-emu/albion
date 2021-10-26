#pragma once
#include <gba/gba.h>

namespace gameboyadvance
{

// common arithmetic and logical operations


// NOTE: arm sets Carry flag on no borrow where as x86 does on borrow
// so we are going to invert it
template<typename T>
inline bool arm_usub_overflow(T v1,T v2) noexcept
{
    return !usub_overflow(v1,v2);
}



// templates for common cpu funcs

template<const bool S>
u32 Cpu::add(u32 v1, u32 v2)
{
    const u32 ans = v1 + v2;
    if constexpr(S)
    {

        flag_v = sadd_overflow(v1,v2);
        flag_c = uadd_overflow(v1,v2);

        set_nz_flag(ans);
    }

    return ans;
}

template<const bool S>
u32 Cpu::adc(u32 v1, u32 v2)
{

    const u32 v3 = flag_c;

    const u32 ans = v1 + v2 + v3;

    if constexpr(S)
    {

        // ^ as if both operations generate an inproper result we will get an expected sign
        const s32 ans_signed = v1 + v2;
        flag_v = sadd_overflow(v1,v2) ^ sadd_overflow(ans_signed,(s32)v3);

        // if either operation overflows we need to set the carry
        const u32 ans_unsigned = v1 + v2;
        flag_c = uadd_overflow(v1,v2) || uadd_overflow(ans_unsigned,v3);

        set_nz_flag(ans);
    }

    return ans;
}

template<const bool S>
u32 Cpu::sub(u32 v1, u32 v2)
{
    
    const u32 ans = v1 - v2;

    if constexpr(S)
    {
        flag_v = sub_overflow(v1,v2);
        flag_c = arm_usub_overflow(v1,v2);

        set_nz_flag(ans);
    }


    return ans;
}

template<const bool S>
u32 Cpu::sbc(u32 v1, u32 v2)
{
    // subtract one from ans if carry is not set
    const u32 v3 = !flag_c;

    const u32 ans = v1 - v2 - v3;
    if constexpr(S)
    {
        // ^ as if both operations generate an inproper result we will get an expected sign
        const s32 ans_signed = v1 - v2;
        flag_v = sub_overflow(v1,v2) ^ sub_overflow(ans_signed,(s32)v3);

        // if both operations overflow we need to set the carry
        const u32 ans_unsigned = v1 - v2;
        flag_c = arm_usub_overflow(v1,v2) && arm_usub_overflow(ans_unsigned,v3);

        set_nz_flag(ans);
    }

    return ans;
}

template<const bool S>
u32 Cpu::logical_and(u32 v1, u32 v2)
{
    const u32 ans = v1 & v2;

    if constexpr(S)
    {
        set_nz_flag(ans);
    }

    return ans;
}

template<const bool S>
u32 Cpu::logical_or(u32 v1, u32 v2)
{
    const u32 ans = v1 | v2;
    if constexpr(S)
    {
        set_nz_flag(ans);
    }
    return ans;
}

template<const bool S>
u32 Cpu::bic(u32 v1, u32 v2)
{
    const u32 ans = v1 & ~v2;
    if constexpr(S)
    {
        set_nz_flag(ans);
    }
    return ans;
}

template<const bool S>
u32 Cpu::logical_eor(u32 v1, u32 v2)
{
    const u32 ans = v1 ^ v2;
    if constexpr(S)
    {
        set_nz_flag(ans);
    }
    return ans;
}

// flag helpers

// set zero flag based on arg
inline void Cpu::set_zero_flag(u32 v)
{
   flag_z = v == 0;
}


inline void Cpu::set_negative_flag(u32 v)
{
    flag_n = static_cast<s32>(v) < 0;
}


// both are set together commonly
// so add a shortcut
inline void Cpu::set_nz_flag(u32 v)
{
    set_zero_flag(v);
    set_negative_flag(v);
}




// set zero flag based on arg
inline void Cpu::set_zero_flag_long(u64 v)
{
    flag_z = v == 0;
}


inline void Cpu::set_negative_flag_long(u64 v)
{
    flag_n = static_cast<s64>(v) < 0;  
}


// both are set together commonly
// so add a shortcut
inline void Cpu::set_nz_flag_long(u64 v)
{
    set_zero_flag_long(v);
    set_negative_flag_long(v);
}

}
