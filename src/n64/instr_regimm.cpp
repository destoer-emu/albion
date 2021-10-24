#include <n64/n64.h>

namespace nintendo64
{

void instr_unknown_regimm(N64 &n64, u32 opcode)
{
    const auto err = fmt::format("[cpu {:16x} {}] regimm unknown opcode {:08x}\n",n64.cpu.pc-4,disass_opcode(opcode,n64.cpu.pc),opcode);
    n64.debug.trace.print();
    throw std::runtime_error(err);        
}

void instr_regimm(N64 &n64, u32 opcode)
{
    instr_regimm_lut[(opcode >> 16) & 0b11111](n64,opcode);
}

void instr_bgezl(N64 &n64, u32 opcode)
{
    const auto rs = get_rs(opcode);

    const auto imm = opcode & 0xffff;

    if(static_cast<s64>(n64.cpu.regs[rs]) >= 0)
    {
        const auto target = compute_branch_addr(n64.cpu.pc,imm);

        write_pc(n64,target);
    }
    
    // discard delay slot
    else
    {
        skip_instr(n64.cpu);
    }
}

void instr_bgezal(N64 &n64, u32 opcode)
{
    const auto rs = get_rs(opcode);

    const auto imm = opcode & 0xffff;

    // link unconditonally
    n64.cpu.regs[RA] = n64.cpu.pc;

    if(static_cast<s64>(n64.cpu.regs[rs]) >= 0)
    {
        const auto target = compute_branch_addr(n64.cpu.pc,imm);
        write_pc(n64,target);
    }
    
    // discard delay slot
    else
    {
        skip_instr(n64.cpu);
    }
}

}