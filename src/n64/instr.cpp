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

void instr_slti(N64 &n64, u32 opcode)
{
    const auto rt = get_rt(opcode);
    const auto rs = get_rs(opcode);

    const auto imm = sign_extend_mips<s64,s16>(opcode & 0xffff);

    n64.cpu.regs[rt] = static_cast<s64>(n64.cpu.regs[rs]) < imm;
}

void instr_addi(N64 &n64, u32 opcode)
{
    const auto rt = get_rt(opcode);
    const auto rs = get_rs(opcode);

    const auto imm = sign_extend_mips<s64,s16>(opcode & 0xffff);

    // 32 bit oper
    const auto ans = sign_extend_mips<s64,s32>(n64.cpu.regs[rs]) + imm;  

    // TODO: speed this up with builtins
    if(did_overflow(n64.cpu.regs[rs],imm,ans))
    {
        unimplemented("addi exception!");
    }  

    else
    {
        n64.cpu.regs[rt] = ans;
    }
}


void instr_ori(N64 &n64, u32 opcode)
{
    const auto rt = get_rt(opcode);
    const auto rs = get_rs(opcode);

    // ori is not sign extended
    const auto imm = opcode & 0xffff;

    n64.cpu.regs[rt] = sign_extend_mips<s64,s32>(n64.cpu.regs[rs]) | imm;    
}

void instr_andi(N64 &n64, u32 opcode)
{
    const auto rt = get_rt(opcode);
    const auto rs = get_rs(opcode);

    // andi is not sign extended
    const auto imm = opcode & 0xffff;

    n64.cpu.regs[rt] = sign_extend_mips<s64,s32>(n64.cpu.regs[rs]) & imm;    
}

void instr_xori(N64 &n64, u32 opcode)
{
    const auto rt = get_rt(opcode);
    const auto rs = get_rs(opcode);

    // xori is not sign extended
    const auto imm = opcode & 0xffff;

    n64.cpu.regs[rt] = sign_extend_mips<s64,s32>(n64.cpu.regs[rs]) ^ imm;    
}

void instr_jal(N64 &n64, u32 opcode)
{
    const auto target = get_target(opcode,n64.cpu.pc);

    n64.cpu.regs[RA] = n64.cpu.pc;

    write_pc(n64,target);
}

void instr_beql(N64 &n64, u32 opcode)
{
    const auto rs = get_rs(opcode);
    const auto rt = get_rt(opcode);

    const auto imm = opcode & 0xffff;

    if(n64.cpu.regs[rs] == n64.cpu.regs[rt])
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

void instr_bnel(N64 &n64, u32 opcode)
{
    const auto rs = get_rs(opcode);
    const auto rt = get_rt(opcode);

    const auto imm = opcode & 0xffff;

    if(n64.cpu.regs[rs] != n64.cpu.regs[rt])
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


void instr_bne(N64 &n64, u32 opcode)
{
    const auto rs = get_rs(opcode);
    const auto rt = get_rt(opcode);

    const auto imm = opcode & 0xffff;

    if(n64.cpu.regs[rs] != n64.cpu.regs[rt])
    {
        const auto target = compute_branch_addr(n64.cpu.pc,imm);

        write_pc(n64,target);
    }
}


void instr_lw(N64 &n64, u32 opcode)
{
    const auto base = get_rs(opcode);
    const auto rt = get_rt(opcode);

    const auto imm = sign_extend_mips<s64,s16>(opcode & 0xffff);

    n64.cpu.regs[rt] = sign_extend_mips<s64,s32>(read_u32(n64,n64.cpu.regs[base] + imm));
}

void instr_sw(N64 &n64, u32 opcode)
{
    const auto base = get_rs(opcode);
    const auto rt = get_rt(opcode);

    const auto imm = sign_extend_mips<s64,s16>(opcode & 0xffff);

    write_u32(n64,n64.cpu.regs[base] + imm,n64.cpu.regs[rt]);
}

}