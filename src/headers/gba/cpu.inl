#pragma once
#include <gba/gba.h>

namespace gameboyadvance
{

// common arithmetic and logical operations


/*
thanks yaed for suggesting use of compilier builtins
*/


template <typename T>
inline bool sub_overflow(T v1,T v2) noexcept
{
    if constexpr(std::is_signed<T>())
    {
#ifdef _MSC_VER
        const T ans = v1 - v2;
        // negate 2nd operand so we can pretend
        // this is like an additon
	    return did_overflow(v1,~v2, ans);
#else
        return __builtin_sub_overflow_p(v1,v2,v1);
#endif
    }

    else
    {
        // on arm the the carry flag is set if there is no borrow
        // this is different to x86 so we cant use builtins here
        return v1 >= v2;
    }
}


template <typename T>
inline bool add_overflow(T v1,T v2) noexcept
{
#ifdef _MSC_VER
	const T ans = v1 + v2;
    if constexpr(std::is_signed<T>())
    {
	    return did_overflow(v1, v2, ans);
    }

    else
    {
        return ans < v1;
    }
#else
    return __builtin_add_overflow_p(v1,v2,v1);
#endif
}

// templates for common cpu funcs

template<const bool S>
uint32_t Cpu::add(uint32_t v1, uint32_t v2)
{
    const uint32_t ans = v1 + v2;
    if constexpr(S)
    {

        flag_v = add_overflow((int32_t)v1,(int32_t)v2);
        flag_c = add_overflow(v1,v2); 

        set_nz_flag(ans);
    }

    return ans;
}

template<const bool S>
uint32_t Cpu::adc(uint32_t v1, uint32_t v2)
{

    const uint32_t v3 = flag_c;

    const uint32_t ans = v1 + v2 + v3;

    if constexpr(S)
    {

        // ^ as if both operations generate an inproper result we will get an expected sign
        const int32_t ans_signed = v1 + v2;
        flag_v = add_overflow((int32_t)v1,(int32_t)v2) ^ add_overflow(ans_signed,(int32_t)v3);

        // if either operation overflows we need to set the carry
        const uint32_t ans_unsigned = v1 + v2;
        flag_c = add_overflow(v1,v2) || add_overflow(ans_unsigned,v3);

        set_nz_flag(ans);
    }

    return ans;
}

template<const bool S>
uint32_t Cpu::sub(uint32_t v1, uint32_t v2)
{
    
    const uint32_t ans = v1 - v2;

    if constexpr(S)
    {
        flag_v = sub_overflow((int32_t)v1,(int32_t)v2);
        flag_c = sub_overflow(v1,v2);

        set_nz_flag(ans);
    }


    return ans;
}

template<const bool S>
uint32_t Cpu::sbc(uint32_t v1, uint32_t v2)
{
    // subtract one from ans if carry is not set
    const uint32_t v3 = !flag_c;

    const uint32_t ans = v1 - v2 - v3;
    if constexpr(S)
    {
        // ^ as if both operations generate an inproper result we will get an expected sign
        const int32_t ans_signed = v1 - v2;
        flag_v = sub_overflow((int32_t)v1,(int32_t)v2) ^ sub_overflow(ans_signed,(int32_t)v3);

        // if both operations overflow we need to set the carry
        const uint32_t ans_unsigned = v1 - v2;
        flag_c = sub_overflow(v1,v2) && sub_overflow(ans_unsigned,v3);

        set_nz_flag(ans);
    }

    return ans;
}

template<const bool S>
uint32_t Cpu::logical_and(uint32_t v1, uint32_t v2)
{
    const uint32_t ans = v1 & v2;

    if constexpr(S)
    {
        set_nz_flag(ans);
    }

    return ans;
}

template<const bool S>
uint32_t Cpu::logical_or(uint32_t v1, uint32_t v2)
{
    const uint32_t ans = v1 | v2;
    if constexpr(S)
    {
        set_nz_flag(ans);
    }
    return ans;
}

template<const bool S>
uint32_t Cpu::bic(uint32_t v1, uint32_t v2)
{
    const uint32_t ans = v1 & ~v2;
    if constexpr(S)
    {
        set_nz_flag(ans);
    }
    return ans;
}

template<const bool S>
uint32_t Cpu::logical_eor(uint32_t v1, uint32_t v2)
{
    const uint32_t ans = v1 ^ v2;
    if constexpr(S)
    {
        set_nz_flag(ans);
    }
    return ans;
}

// flag helpers

// set zero flag based on arg
inline void Cpu::set_zero_flag(uint32_t v)
{
   flag_z =  v == 0;
}


inline void Cpu::set_negative_flag(uint32_t v)
{
    flag_n = is_set(v,31);
}


// both are set together commonly
// so add a shortcut
inline void Cpu::set_nz_flag(uint32_t v)
{
    set_zero_flag(v);
    set_negative_flag(v);
}




// set zero flag based on arg
inline void Cpu::set_zero_flag_long(uint64_t v)
{
    flag_z = v == 0;
}


inline void Cpu::set_negative_flag_long(uint64_t v)
{
    flag_n = is_set(v,63);  
}


// both are set together commonly
// so add a shortcut
inline void Cpu::set_nz_flag_long(uint64_t v)
{
    set_zero_flag_long(v);
    set_negative_flag_long(v);
}

}
