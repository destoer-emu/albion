#include <n64/n64.h>

namespace nintendo64
{

f32 bit_cast_float(s32 v)
{
    static_assert(sizeof(f32) == sizeof(s32));

    f32 out = 0.0;
    memcpy(&out,&v,sizeof(s32));
    return out;
}

f64 bit_cast_double(s64 v)
{
    static_assert(sizeof(f64) == sizeof(s64));

    f64 out = 0.0;
    memcpy(&out,&v,sizeof(u64));
    return out;
}

s64 bit_cast_from_double(f64 v)
{
    static_assert(sizeof(f64) == sizeof(s64));

    u64 out = 0;
    memcpy(&out,&v,sizeof(f64));
    return out;    
}

s32 bit_cast_from_float(f32 v)
{
    static_assert(sizeof(f32) == sizeof(s32));

    u32 out = 0;
    memcpy(&out,&v,sizeof(f32));
    return out;    
}


void instr_unknown_cop1(N64 &n64, const Opcode &opcode)
{
    const auto err = std::format("[cpu {:16x} {}] unknown cop1 opcode {:08x} : {:08x}\n",
        n64.cpu.pc,disass_n64(n64,opcode,n64.cpu.pc),(opcode.op >> 21) & 0b11111,opcode.op);

    n64.debug.trace.print();
    throw std::runtime_error(err);    
}

template<const b32 debug>
void instr_COP1(N64 &n64, const Opcode &opcode)
{
    // coprocesor unusable
    if(!cop1_usable(n64))
    {
        return;
    }

    const u32 offset = calc_cop1_table_offset(opcode);

    call_handler<debug>(n64,opcode,offset);
}

template<const b32 debug>
void instr_lwc1(N64 &n64, const Opcode &opcode)
{
    // coprocesor unusable
    if(!cop1_usable(n64))
    {
        return;
    }

    const auto base = opcode.rs;
    const u32 ft = get_ft(opcode);

    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);


    const u32 v = read_u32<debug>(n64,n64.cpu.regs[base] + imm);
    const f32 f = bit_cast_float(v);

    write_cop1_reg(n64,ft,f);    
}

template<const b32 debug>
void instr_ldc1(N64 &n64, const Opcode &opcode)
{
    // coprocesor unusable
    if(!cop1_usable(n64))
    {
        return;
    }

    const auto base = opcode.rs;
    const u32 ft = get_ft(opcode);

    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);


    const u64 v = read_u64<debug>(n64,n64.cpu.regs[base] + imm);
    const f64 f = bit_cast_double(v);

    write_cop1_reg(n64,ft,f);   
}

template<const b32 debug>
void instr_swc1(N64 &n64, const Opcode &opcode)
{
    // coprocesor unusable
    if(!cop1_usable(n64))
    {
        return;
    }

    const auto base = opcode.rs;
    const u32 ft = get_ft(opcode);

    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);


    const f32 f = read_cop1_reg(n64,ft);
    const s32 v = bit_cast_from_float(f);

    write_u32<debug>(n64,n64.cpu.regs[base] + imm,v);
}

template<const b32 debug>
void instr_sdc1(N64 &n64, const Opcode &opcode)
{
    // coprocesor unusable
    if(!cop1_usable(n64))
    {
        return;
    }

    const auto base = opcode.rs;
    const u32 ft = get_ft(opcode);

    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);


    const f64 f = read_cop1_reg(n64,ft);
    const s64 v = bit_cast_from_double(f);

    write_u64<debug>(n64,n64.cpu.regs[base] + imm,v);
}

void instr_cfc1(N64& n64, const Opcode &opcode)
{
    const u32 fs = get_fs(opcode);
    n64.cpu.regs[opcode.rt] = read_cop1_control(n64,fs);
}

void instr_ctc1(N64& n64, const Opcode &opcode)
{
    const u32 fs = get_fs(opcode);
    write_cop1_control(n64,fs,n64.cpu.regs[opcode.rt]);
}

void instr_mtc1(N64& n64, const Opcode &opcode)
{
    const u32 fs = get_fs(opcode);
    
    const f32 f = bit_cast_float(n64.cpu.regs[opcode.rt]);
    write_cop1_reg(n64,fs,f);
}

void instr_mfc1(N64& n64, const Opcode& opcode)
{
    const u32 fs = get_fs(opcode);

    const s32 w = bit_cast_from_float(read_cop1_reg(n64,fs));
    n64.cpu.regs[opcode.rt] = sign_extend_type<s64,s32>(w);
}

}