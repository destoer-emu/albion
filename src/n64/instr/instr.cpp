#include <n64/n64.h>

#include <n64/instr/instr_r.cpp>
#include <n64/instr/instr_regimm.cpp>
#include <n64/instr/instr_cop0.cpp>
#include <n64/instr/instr_cop1.cpp>
#include <n64/instr/instr_cop2.cpp>

namespace nintendo64
{

void instr_unknown_opcode(N64 &n64, const Opcode &opcode)
{
    const auto err = std::format("[cpu {:16x} {}] unknown opcode {:08x}\n",n64.cpu.pc-4,disass_n64(n64,opcode,n64.cpu.pc),opcode.op);
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

    n64.cpu.regs[opcode.rt] = s64(n64.cpu.regs[opcode.rs]) < imm;
}

void instr_sltiu(N64 &n64, const Opcode &opcode)
{
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);

    n64.cpu.regs[opcode.rt] = n64.cpu.regs[opcode.rs] < u64(imm);
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
    if(did_overflow(s64(n64.cpu.regs[opcode.rs]),imm,ans))
    {
        unimplemented("daddi exception!");
    }  

    else
    {
        n64.cpu.regs[opcode.rt] = ans;
    }
}



void instr_daddiu(N64 &n64, const Opcode &opcode)
{
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);

    // 64 bit oper no overflow
    n64.cpu.regs[opcode.rt]  = n64.cpu.regs[opcode.rs] + imm;  
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

    n64.cpu.regs[RA] = n64.cpu.pc_next;

    write_pc(n64,target);
}

void instr_j(N64 &n64, const Opcode &opcode)
{
    const auto target = get_target(opcode.op,n64.cpu.pc);

    // trivial waitloop
    if(target == n64.cpu.pc - 4)
    {
        n64.scheduler.skip_to_event();
    }

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
    if(s64(n64.cpu.regs[opcode.rs]) <= 0)
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
    const auto target = compute_branch_addr(n64.cpu.pc,opcode.imm);

    // branch to self (trivial waitloop)
    if(opcode.rs == opcode.rt && target == (n64.cpu.pc - 4))
    {
        puts("waitloop");
        exit(1);
    }


    if(n64.cpu.regs[opcode.rs] != n64.cpu.regs[opcode.rt])
    {
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
    
    const auto target = compute_branch_addr(n64.cpu.pc,opcode.imm);

    // branch to self (trivial waitloop)
    if(opcode.rs == opcode.rt && target == (n64.cpu.pc - 4))
    {
        puts("waitloop");
        exit(1);
    }

    if(n64.cpu.regs[opcode.rs] == n64.cpu.regs[opcode.rt])
    { 
        write_pc(n64,target);
    }
}


void instr_cache(N64 &n64, const Opcode &opcode)
{
    UNUSED(n64); UNUSED(opcode);

    // ignore cache operations for now
}

template<const b32 debug>
void instr_lb(N64 &n64, const Opcode &opcode)
{
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);

    n64.cpu.regs[opcode.rt] = sign_extend_mips<s64,s8>(read_u8<debug>(n64,n64.cpu.regs[base] + imm));
}


template<const b32 debug>
void instr_lw(N64 &n64, const Opcode &opcode)
{
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);

    n64.cpu.regs[opcode.rt] = sign_extend_mips<s64,s32>(read_u32<debug>(n64,n64.cpu.regs[base] + imm));
}

template<const b32 debug>
void instr_ld(N64 &n64, const Opcode &opcode)
{
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);

    n64.cpu.regs[opcode.rt] = read_u64<debug>(n64,n64.cpu.regs[base] + imm);
}

template<const b32 debug>
void instr_ldl(N64 &n64, const Opcode &opcode)
{
    instr_unknown_opcode(n64,opcode);
}

template<const b32 debug>
void instr_lwu(N64 &n64, const Opcode &opcode)
{
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);

    // not sign extended
    n64.cpu.regs[opcode.rt] = read_u32<debug>(n64,n64.cpu.regs[base] + imm);
}

template<const b32 debug>
void instr_lwl(N64 &n64, const Opcode &opcode)
{
    instr_unknown_opcode(n64,opcode);
}

template<const b32 debug>
void instr_lwr(N64 &n64, const Opcode &opcode)
{
    instr_unknown_opcode(n64,opcode);
}

template<const b32 debug>
void instr_sw(N64 &n64, const Opcode &opcode)
{
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);

    write_u32<debug>(n64,n64.cpu.regs[base] + imm,n64.cpu.regs[opcode.rt]);
}

template<const b32 debug>
void instr_swr(N64 &n64, const Opcode &opcode)
{
    instr_unknown_opcode(n64,opcode);
}

template<const b32 debug>
void instr_swl(N64 &n64, const Opcode &opcode)
{
    instr_unknown_opcode(n64,opcode);
}

template<const b32 debug>
void instr_sh(N64 &n64, const Opcode &opcode)
{
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);

    write_u16<debug>(n64,n64.cpu.regs[base] + imm,n64.cpu.regs[opcode.rt]);
}

template<const b32 debug>
void instr_sd(N64 &n64, const Opcode &opcode)
{
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);

    write_u64<debug>(n64,n64.cpu.regs[base] + imm,n64.cpu.regs[opcode.rt]);
}

template<const b32 debug>
void instr_sdl(N64 &n64, const Opcode &opcode)
{
    instr_unknown_opcode(n64,opcode);
}

template<const b32 debug>
void instr_sdr(N64 &n64, const Opcode &opcode)
{
    instr_unknown_opcode(n64,opcode);
}

template<const b32 debug>
void instr_ll(N64 &n64, const Opcode &opcode)
{
    instr_unknown_opcode(n64,opcode);
}

template<const b32 debug>
void instr_lld(N64 &n64, const Opcode &opcode)
{
    instr_unknown_opcode(n64,opcode);
}

template<const b32 debug>
void instr_ldr(N64 &n64, const Opcode &opcode)
{
    instr_unknown_opcode(n64,opcode);
}

template<const b32 debug>
void instr_lbu(N64 &n64, const Opcode &opcode)
{
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);

    n64.cpu.regs[opcode.rt] = read_u8<debug>(n64,n64.cpu.regs[base] + imm);
}

template<const b32 debug>
void instr_sb(N64 &n64, const Opcode &opcode)
{
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);

    write_u8<debug>(n64,n64.cpu.regs[base] + imm,n64.cpu.regs[opcode.rt]);
}

template<const b32 debug>
void instr_lhu(N64 &n64, const Opcode &opcode)
{
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);

    n64.cpu.regs[opcode.rt] = read_u16<debug>(n64,n64.cpu.regs[base] + imm);
}

template<const b32 debug>
void instr_lh(N64 &n64, const Opcode &opcode)
{
    instr_unknown_opcode(n64,opcode);
}


void instr_bgtz(N64 &n64, const Opcode &opcode)
{
    if(s64(n64.cpu.regs[opcode.rs]) > 0)
    {
        const auto target = compute_branch_addr(n64.cpu.pc,opcode.imm);

        write_pc(n64,target);
    }
}

void instr_blez(N64& n64, const Opcode& opcode)
{
    if(s64(n64.cpu.regs[opcode.rs]) <= 0)
    {
        const auto target = compute_branch_addr(n64.cpu.pc,opcode.imm);

        write_pc(n64,target);
    }
}

void instr_bgtzl(N64& n64, const Opcode& opcode)
{
    instr_unknown_opcode(n64,opcode);
}

}