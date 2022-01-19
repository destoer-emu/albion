#include <n64/n64.h>

namespace nintendo64
{

void instr_unknown(N64 &n64, const Opcode &opcode)
{
    const auto err = fmt::format("[cpu {:16x} {}] unknown opcode {:08x}\n",n64.cpu.pc-4,disass_opcode(opcode,n64.cpu.pc),opcode.op);
    n64.debug.trace.print();
    throw std::runtime_error(err);    
}


void instr_lui(N64 &n64, const Opcode &opcode)
{
    // virtually everything on mips has to be sign extended
    n64.cpu.regs[opcode.rt] = sign_extend_mips<s64,s32>(opcode.imm << 16);
}

void instr_addiu(N64 &n64, const Opcode &opcode)
{
    const auto imm = sign_extend_mips<s32,s16>(opcode.imm);

    // addiu (oper is 32 bit, no exceptions thrown)
    n64.cpu.regs[opcode.rt] = sign_extend_mips<s64,s32>(u32(n64.cpu.regs[opcode.rs]) + imm);
}

void instr_slti(N64 &n64, const Opcode &opcode)
{
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);

    n64.cpu.regs[opcode.rt] = static_cast<s64>(n64.cpu.regs[opcode.rs]) < imm;
}

void instr_addi(N64 &n64, const Opcode &opcode)
{
    const auto imm = sign_extend_mips<s32,s16>(opcode.imm);

    // 32 bit oper
    const auto ans = sign_extend_mips<s64,s32>(u32(n64.cpu.regs[opcode.rs]) + imm);  

    // TODO: speed this up with builtins
    if(did_overflow(s32(n64.cpu.regs[opcode.rs]),s32(imm),s32(ans)))
    {
        unimplemented("addi exception!");
    }  

    else
    {
        n64.cpu.regs[opcode.rt] = ans;
    }
}


void instr_daddi(N64 &n64, const Opcode &opcode)
{
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);

    // 64 bit oper
    const s64 ans = n64.cpu.regs[opcode.rs] + imm;  

    // TODO: speed this up with builtins
    if(did_overflow(n64.cpu.regs[opcode.rs],imm,ans))
    {
        unimplemented("daddi exception!");
    }  

    else
    {
        n64.cpu.regs[opcode.rt] = ans;
    }
}


void instr_ori(N64 &n64, const Opcode &opcode)
{
    // ori is not sign extended
    n64.cpu.regs[opcode.rt] = n64.cpu.regs[opcode.rs] | opcode.imm;    
}

void instr_andi(N64 &n64, const Opcode &opcode)
{
    // andi is not sign extended
    n64.cpu.regs[opcode.rt] = n64.cpu.regs[opcode.rs] & opcode.imm;    
}

void instr_xori(N64 &n64, const Opcode &opcode)
{
    // xori is not sign extended
    n64.cpu.regs[opcode.rt] = n64.cpu.regs[opcode.rs] ^ opcode.imm;    
}

void instr_jal(N64 &n64, const Opcode &opcode)
{
    const auto target = get_target(opcode.op,n64.cpu.pc);

    n64.cpu.regs[RA] = n64.cpu.pc;

    write_pc(n64,target);
}

void instr_j(N64 &n64, const Opcode &opcode)
{
    const auto target = get_target(opcode.op,n64.cpu.pc);
    write_pc(n64,target);
}


void instr_beql(N64 &n64, const Opcode &opcode)
{
    if(n64.cpu.regs[opcode.rs] == n64.cpu.regs[opcode.rt])
    {
        const auto target = compute_branch_addr(n64.cpu.pc,opcode.imm);

        write_pc(n64,target);
    }
    
    // discard delay slot
    else
    {
        skip_instr(n64.cpu);
    }
}

void instr_blezl(N64 &n64, const Opcode &opcode)
{
    if(static_cast<s64>(n64.cpu.regs[opcode.rs]) <= 0)
    {
        const auto target = compute_branch_addr(n64.cpu.pc,opcode.imm);

        write_pc(n64,target);
    }
    
    // discard delay slot
    else
    {
        skip_instr(n64.cpu);
    }
}

void instr_bnel(N64 &n64, const Opcode &opcode)
{
    if(n64.cpu.regs[opcode.rs] != n64.cpu.regs[opcode.rt])
    {
        const auto target = compute_branch_addr(n64.cpu.pc,opcode.imm);

        write_pc(n64,target);
    }
    
    // discard delay slot
    else
    {
        skip_instr(n64.cpu);
    }
}


void instr_bne(N64 &n64, const Opcode &opcode)
{
    if(n64.cpu.regs[opcode.rs] != n64.cpu.regs[opcode.rt])
    {
        const auto target = compute_branch_addr(n64.cpu.pc,opcode.imm);

        write_pc(n64,target);
    }
}

void instr_beq(N64 &n64, const Opcode &opcode)
{
    if(n64.cpu.regs[opcode.rs] == n64.cpu.regs[opcode.rt])
    {
        const auto target = compute_branch_addr(n64.cpu.pc,opcode.imm);

        write_pc(n64,target);
    }
}


void instr_cache(N64 &n64, const Opcode &opcode)
{
    UNUSED(n64); UNUSED(opcode);

    // ignore cache operations for now
}

void instr_lb(N64 &n64, const Opcode &opcode)
{
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);

    n64.cpu.regs[opcode.rt] = sign_extend_mips<s64,s8>(read_u8(n64,n64.cpu.regs[base] + imm));
}



void instr_lw(N64 &n64, const Opcode &opcode)
{
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);

    n64.cpu.regs[opcode.rt] = sign_extend_mips<s64,s32>(read_u32(n64,n64.cpu.regs[base] + imm));
}

void instr_ld(N64 &n64, const Opcode &opcode)
{
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);

    n64.cpu.regs[opcode.rt] = read_u64(n64,n64.cpu.regs[base] + imm);
}


void instr_lwu(N64 &n64, const Opcode &opcode)
{
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);

    // not sign extended
    n64.cpu.regs[opcode.rt] = read_u32(n64,n64.cpu.regs[base] + imm);
}

void instr_sw(N64 &n64, const Opcode &opcode)
{
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);

    write_u32(n64,n64.cpu.regs[base] + imm,n64.cpu.regs[opcode.rt]);
}

void instr_sh(N64 &n64, const Opcode &opcode)
{
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);

    write_u16(n64,n64.cpu.regs[base] + imm,n64.cpu.regs[opcode.rt]);
}

void instr_sd(N64 &n64, const Opcode &opcode)
{
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);

    write_u64(n64,n64.cpu.regs[base] + imm,n64.cpu.regs[opcode.rt]);
}

void instr_lbu(N64 &n64, const Opcode &opcode)
{
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);

    n64.cpu.regs[opcode.rt] = read_u8(n64,n64.cpu.regs[base] + imm);
}

void instr_sb(N64 &n64, const Opcode &opcode)
{
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);

    write_u8(n64,n64.cpu.regs[base] + imm,n64.cpu.regs[opcode.rt]);
}


void instr_bgtz(N64 &n64, const Opcode &opcode)
{
    if(static_cast<s64>(n64.cpu.regs[opcode.rs]) > 0)
    {
        const auto target = compute_branch_addr(n64.cpu.pc,opcode.imm);

        write_pc(n64,target);
    }
    
    // discard delay slot
    else
    {
        skip_instr(n64.cpu);
    }
}

}