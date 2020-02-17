#pragma once
#include <destoer-emu/lib.h>

// user and system share the same registers
enum class cpu_mode 
{
    fiq = 0,supervisor = 1,abort=2,
    irq=3, undefined=4, user=5, system=6
};



constexpr int ARM_WORD_SIZE = 4; 
constexpr int ARM_HALF_SIZE = 2;

enum class access_size
{
    byte = 0,half,word
};

constexpr uint32_t access_sizes[3] = {1,ARM_HALF_SIZE,ARM_WORD_SIZE};


enum class shift_type
{
    lsl=0,lsr,asr,ror
};

constexpr int R0 = 0;
constexpr int R1 = 1;
constexpr int R2 = 2;
constexpr int R3 = 3;
constexpr int R4 = 4;
constexpr int R5 = 5;
constexpr int R6 = 6;
constexpr int R7 = 7;
constexpr int R8 = 8;
constexpr int R9 = 9;
constexpr int R10 = 10;
constexpr int R11 = 11;
constexpr int R12 = 12;
constexpr int SP = 13; // stack pointer
constexpr int LR = 14; // return address (link register)
constexpr int PC = 15; // program counter



// register names
extern const char *user_regs_names[16];

extern const char *fiq_banked_names[5];

extern const char *hi_banked_names[5][2];

extern const char *status_banked_names[5];

extern const char *mode_names[7];






// flags
constexpr int N_BIT = 31; // negative
constexpr int Z_BIT = 30; // zero
constexpr int C_BIT = 29; // carry
constexpr int V_BIT = 28; // overflow


enum class arm_cond
{
    eq=0,ne=1,cs=2,cc=3,mi=4,pl=5,vs=6,
    vc=7,hi=8,ls=9,ge=10,lt=11,gt=12,le=13,al=14
};



inline uint32_t get_arm_opcode_bits(uint32_t instr)
{
    return ((instr >> 4) & 0xf) | ((instr >> 16) & 0xff0);    
}

inline uint8_t get_thumb_opcode_bits(uint16_t instr)
{
    return ((instr >> 8) & 0xff);    
}

// operand two immediate is produced
// with a 8 bit imm rotated right
// by a shift value * 2
inline uint32_t get_arm_operand2_imm(uint32_t opcode)
{
    uint8_t imm = opcode & 0xff;
    const int shift = (opcode >> 8) & 0xf;
    return rotr(imm,shift*2);   
}


inline uint32_t get_cpsr_mode_bits(cpu_mode mode)
{
    switch(mode)
    {
        case cpu_mode::user: return 0b10000;
        case cpu_mode::fiq: return 0b10001;
        case cpu_mode::irq: return 0b10010; 
        case cpu_mode::supervisor: return 0b10011;
        case cpu_mode::abort: return 0b10111;
        case cpu_mode::undefined: return 0b11011;
        case cpu_mode::system: return 0b11111;
        default:
        { 
            printf("unknown mode for cpsr flag: %08x\n",static_cast<int>(mode));
            exit(1);
        }        
    }    
}


// TODO these aint handling edge cases
// with large shifts
// http://www.keil.com/support/man/docs/armasm/armasm_dom1361289852998.htm


inline uint32_t lsl(uint32_t v, uint32_t n, bool &carry)
{
    if(!n) return v;

    else if(n >= 32)
    {

        if(n >= 33)
        {
            carry = false;
        }

        else
        {
            carry = is_set(v, 0);
        }


        return 0;
    }


    carry = is_set(v, 32-n);

    return v << n;
}

inline uint32_t lsr(uint32_t v, uint32_t n, bool &carry,bool immediate)
{
    if(n >= 32 || (n == 0 && immediate))
    {

        if(n >= 33)
        {
            carry = false;
        }

        else
        {
            carry = is_set(v, 31);
        }
        return 0;
    }

    else if(n == 0)
    {
        return v;
    }


    carry = is_set(v,n-1);

    return v >> n;
}

inline uint32_t asr(uint32_t v, uint32_t n, bool &carry,bool immediate)
{
    if(n >= 32 || (n == 0 && immediate))
    {
        carry = is_set(v,31);
        return carry * 0xffffffff;
    }

    else if(n == 0)
    {
        return v;
    }

    carry = is_set(v,n-1);

    // force an asr
    int32_t x = (int32_t)v;

    x >>= n;

    return x;
}

// how the heck are carrys defined on this
inline uint32_t ror(uint32_t v, uint32_t n, bool &carry,bool immediate)
{
    if(n == 0 && immediate) // ror #0 = rrx  <-- need to check this
    {
        bool c = is_set(v,0);
        v >>= 1;
        v = carry? set_bit(v,31) : deset_bit(v,31);
        carry = c;
        return v;
    }

    else if(n == 0)
    {
        return v;
    }

    v = rotr(v,n-1);
    carry = is_set(v,0);
    return rotr(v,1);
}

// barrel shifter
inline uint32_t barrel_shift(shift_type type,uint32_t v, uint32_t n, bool &carry,bool immediate)
{
    switch(type)
    {
        case shift_type::lsl: return lsl(v,n,carry); break;
        case shift_type::lsr: return lsr(v,n,carry,immediate); break;
        case shift_type::asr: return asr(v,n,carry,immediate); break;
        case shift_type::ror: return ror(v,n,carry,immediate); break;
    }
}

