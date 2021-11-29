#include <n64/n64.h>

namespace nintendo64
{

void instr_unknown_r(N64 &n64, const Opcode &opcode)
{
    const auto err = fmt::format("[cpu {:16x} {}] unknown r opcode {:08x}\n",n64.cpu.pc,disass_opcode(opcode,n64.cpu.pc),opcode.op & 0b11111);
    n64.debug.trace.print();
    throw std::runtime_error(err);        
}

void instr_r_fmt(N64 &n64, const Opcode &opcode)
{
    instr_r_lut[opcode.op & 0b111111](n64,opcode);
}

void instr_sll(N64 &n64, const Opcode &opcode)
{
    const auto shamt = get_shamt(opcode.op);


    n64.cpu.regs[opcode.rd] = sign_extend_mips<s64,s32>(n64.cpu.regs[opcode.rt] << shamt);
}

void instr_sllv(N64 &n64, const Opcode &opcode)
{
    n64.cpu.regs[opcode.rd] = sign_extend_mips<s64,s32>(n64.cpu.regs[opcode.rt] << (n64.cpu.regs[opcode.rs] & 0b11111));    
}

void instr_srl(N64 &n64, const Opcode &opcode)
{
    const auto shamt = get_shamt(opcode.op);

    n64.cpu.regs[opcode.rd] = sign_extend_mips<s64,s32>(n64.cpu.regs[opcode.rt] >> shamt);    
}

void instr_srlv(N64 &n64, const Opcode &opcode)
{
    n64.cpu.regs[opcode.rd] = sign_extend_mips<s64,s32>(n64.cpu.regs[opcode.rt] >> (n64.cpu.regs[opcode.rs] & 0b11111));    
}

void instr_sltu(N64 &n64, const Opcode &opcode)
{
    n64.cpu.regs[opcode.rd] = n64.cpu.regs[opcode.rs] < n64.cpu.regs[opcode.rt];    
}

void instr_slt(N64 &n64, const Opcode &opcode)
{
    n64.cpu.regs[opcode.rd] = static_cast<s64>(n64.cpu.regs[opcode.rs]) < static_cast<s64>(n64.cpu.regs[opcode.rt]);    
}

void instr_subu(N64 &n64, const Opcode &opcode)
{
    // does not trap on overflow
    n64.cpu.regs[opcode.rd] = sign_extend_mips<s64,s32>(n64.cpu.regs[opcode.rs] - n64.cpu.regs[opcode.rt]);
}

void instr_addu(N64 &n64, const Opcode &opcode)
{
    // does not trap on overflow
    n64.cpu.regs[opcode.rd] = sign_extend_mips<s64,s32>(n64.cpu.regs[opcode.rs] + n64.cpu.regs[opcode.rt]);
}

void instr_add(N64 &n64, const Opcode &opcode)
{
    // does not trap on overflow
    const auto ans =  sign_extend_mips<s64,s32>(n64.cpu.regs[opcode.rs] + n64.cpu.regs[opcode.rt]);

    if(did_overflow(n64.cpu.regs[opcode.rs],n64.cpu.regs[opcode.rt],ans))
    {
        unimplemented("instr_add overflow");
    }

    else
    {
        n64.cpu.regs[opcode.rd] = ans;
    }
}


void instr_and(N64 &n64, const Opcode &opcode)
{
    n64.cpu.regs[opcode.rd] = n64.cpu.regs[opcode.rs] & n64.cpu.regs[opcode.rt];
}



void instr_multu(N64 &n64, const Opcode &opcode)
{
    const u64 res = static_cast<u32>(n64.cpu.regs[opcode.rs]) * static_cast<u32>(n64.cpu.regs[opcode.rt]);

    n64.cpu.lo = sign_extend_mips<s64,s32>(res & 0xffffffff);
    n64.cpu.hi = sign_extend_mips<s64,s32>((res >> 32) & 0xffffffff);
}

void instr_mflo(N64 &n64, const Opcode &opcode)
{
    n64.cpu.regs[opcode.rd] = n64.cpu.lo;
}

void instr_or(N64 &n64, const Opcode &opcode)
{
    n64.cpu.regs[opcode.rd] = n64.cpu.regs[opcode.rs] | n64.cpu.regs[opcode.rt];
}

void instr_xor(N64 &n64, const Opcode &opcode)
{
    n64.cpu.regs[opcode.rd] = n64.cpu.regs[opcode.rs] ^ n64.cpu.regs[opcode.rt];
}


void instr_jr(N64 &n64, const Opcode &opcode)
{
    if((n64.cpu.regs[opcode.rs] & 0b11) != 0)
    {
        unimplemented("jr address exception");
    }

    else
    {
        write_pc(n64,n64.cpu.regs[opcode.rs]);
    }
}


}