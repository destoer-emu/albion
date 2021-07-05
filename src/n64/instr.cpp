#include <n64/n64.h>

namespace nintendo64
{

void instr_unknown(N64 &n64, u32 opcode)
{
    const auto err = fmt::format("[cpu {:16x} {}] unknown opcode {:08x}\n",n64.cpu.pc,disass_opcode(opcode,n64.cpu.pc),opcode);
    n64.debug.trace.print();
    throw std::runtime_error(err);    
}


void instr_lui(N64 &n64, u32 opcode)
{
    const auto rt = get_rt(opcode);

    // virtually everything on mips has to be sign extended
    n64.cpu.regs[rt] = sign_extend_mips<s64,s32>((opcode & 0xffff) << 16);
}

void instr_addiu(N64 &n64, u32 opcode)
{
    const auto rt = get_rt(opcode);
    const auto rs = get_rs(opcode);

    const auto imm = sign_extend_mips<s64,s16>(opcode & 0xffff);

    // addiu (oper is 32 bit, no exceptions thrown)
    n64.cpu.regs[rt] = sign_extend_mips<s64,s32>(n64.cpu.regs[rs]) + imm;
}



// TODO: how do we do the pc_old trick to get branch delay slots working?
void instr_bne(N64 &n64, u32 opcode)
{
    const auto rs = get_rs(opcode);
    const auto rt = get_rt(opcode);

    const auto imm = opcode & 0xffff;

    if(n64.cpu.regs[rs] != n64.cpu.regs[rt])
    {
        n64.cpu.pc = compute_branch_addr(n64.cpu.pc,imm);
    }
}

void instr_lw(N64 &n64, u32 opcode)
{
    const auto base = get_rs(opcode);
    const auto rt = get_rt(opcode);

    const auto imm = sign_extend_mips<s64,s16>(opcode & 0xffff);

    n64.cpu.regs[rt] = sign_extend_mips<s64,s32>(read_u32(n64,n64.cpu.regs[base] + imm));
}

}