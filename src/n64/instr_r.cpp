#include <n64/n64.h>

namespace nintendo64
{

void instr_unknown_r(N64 &n64, u32 opcode)
{
    const auto err = fmt::format("[cpu {:16x} {}] unknown r opcode {:08x}\n",n64.cpu.pc,disass_opcode(opcode,n64.cpu.pc),opcode & 0b11111);
    n64.debug.trace.print();
    throw std::runtime_error(err);        
}

void instr_r_fmt(N64 &n64, u32 opcode)
{
    instr_r_lut[opcode  & 0b111111](n64,opcode);
}

void instr_sll(N64 &n64, u32 opcode)
{
    const auto rt = get_rt(opcode);
    const auto rd = get_rd(opcode);

    const auto shamt = get_shamt(opcode);


    n64.cpu.regs[rd] = sign_extend_mips<s64,s32>(n64.cpu.regs[rt] << shamt);
}

void instr_or(N64 &n64, u32 opcode)
{
    const auto rt = get_rt(opcode);
    const auto rd = get_rd(opcode);
    const auto rs = get_rs(opcode);


    n64.cpu.regs[rd] = sign_extend_mips<s64,s32>(n64.cpu.regs[rs] | n64.cpu.regs[rt]);
}

void instr_jr(N64 &n64, u32 opcode)
{
    const auto rs = get_rs(opcode);

    if((n64.cpu.regs[rs] & 0b11) != 0)
    {
        unimplemented("jr address exception");
    }

    else
    {
        n64.cpu.pc = n64.cpu.regs[rs];
    }
}


}