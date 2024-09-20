#include <n64/n64.h>

namespace nintendo64
{

template<typename FUNC>
void instr_branch(N64& n64, const Opcode& opcode, FUNC func)
{
    if(func(n64,opcode))
    {
        const auto target = compute_branch_addr(n64.cpu.pc,opcode.imm);

        write_pc(n64,target);
    }
}

template<typename FUNC>
void instr_branch_likely(N64& n64, const Opcode& opcode, FUNC func)
{
    if(func(n64,opcode))
    {
        const auto target = compute_branch_addr(n64.cpu.pc,opcode.imm);

        write_pc(n64,target);
    }

    // skip delay slot
    else
    {
        skip_instr(n64.cpu);
    }   
}

}

#include "instr/instr_r.cpp"
#include "instr/instr_regimm.cpp"
#include "instr/instr_cop0.cpp"
#include "instr/instr_cop1.cpp"
#include "instr/instr_cop2.cpp"
#include "instr/instr_float.cpp"

namespace nintendo64
{

void instr_unknown_opcode(N64 &n64, const Opcode &opcode)
{
    const auto err = fmt::format("[cpu {:16x} {}] unknown opcode {:08x}, idx {}\n",n64.cpu.pc-4,disass_n64(n64,opcode,n64.cpu.pc),opcode.op, instr_idx(opcode, beyond_all_repair::MIPS4));
    n64.debug.trace.print();
    throw std::runtime_error(err);    
}

void instr_invalid_opcode_cvt_d_d(N64& n64, const Opcode& opcode)
{
    UNUSED(opcode);
    const auto err = fmt::format("[cpu {:16x} cvt.d.d $f0 $f0] invalid opcode format for cvt\n", n64.cpu.pc-4);
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

    n64.cpu.regs[beyond_all_repair::RA] = n64.cpu.pc_next;

    write_pc(n64,target);
}

void instr_j(N64 &n64, const Opcode &opcode)
{
    const auto target = get_target(opcode.op,n64.cpu.pc);

    // trivial waitloop
    if(target == n64.cpu.pc - 4)
    {
        while(!n64.cpu.interrupt && !n64.rdp.frame_done)
        {
            n64.scheduler.skip_to_event();
        }
    }

    write_pc(n64,target);
}


void instr_beql(N64 &n64, const Opcode &opcode)
{
    instr_branch_likely(n64,opcode,[](N64& n64, const Opcode& opcode)
    {
        return n64.cpu.regs[opcode.rs] == n64.cpu.regs[opcode.rt];
    });
}

void instr_blezl(N64 &n64, const Opcode &opcode)
{
    instr_branch_likely(n64,opcode,[](N64& n64, const Opcode& opcode)
    {
        return s64(n64.cpu.regs[opcode.rs]) <= 0;
    });
}

void instr_bnel(N64 &n64, const Opcode &opcode)
{
    instr_branch_likely(n64,opcode,[](N64& n64, const Opcode& opcode)
    {
        return n64.cpu.regs[opcode.rs] != n64.cpu.regs[opcode.rt];
    });
}


void instr_bne(N64 &n64, const Opcode &opcode)
{
    instr_branch(n64,opcode,[](N64& n64, const Opcode& opcode)
    {
        return n64.cpu.regs[opcode.rs] != n64.cpu.regs[opcode.rt];
    });
}

void instr_beq(N64 &n64, const Opcode &opcode)
{
    instr_branch(n64,opcode,[](N64& n64, const Opcode& opcode)
    {
        return n64.cpu.regs[opcode.rs] == n64.cpu.regs[opcode.rt];
    });
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
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);
    u64 addr = (n64.cpu.regs[base] + imm);

    const u32 offset = (addr & 7);
    const u64 mask = u64(0xffff'ffff'ffff'ffff) << (offset * 8);

    // 'rotate' like an unaligned arm load
    u64 v = read_u64<debug>(n64,addr) << (offset * 8);

    // combine reg with load
    v = (v & mask) | (u64(n64.cpu.regs[opcode.rt]) & ~mask);

    n64.cpu.regs[opcode.rt] = v;    
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
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);
    u64 addr = (n64.cpu.regs[base] + imm);

    const u32 offset = (addr & 3);
    const u32 mask = u32(0xffff'ffff) << (offset * 8);

    // 'rotate' like an unaligned arm load
    u32 v = read_u32<debug>(n64,addr) << (offset * 8);

    // combine reg with load
    v = (v & mask) | (u32(n64.cpu.regs[opcode.rt]) & ~mask);

    // sign extend ans back out;
    n64.cpu.regs[opcode.rt] = sign_extend_mips<s64,s32>(v);
}

template<const b32 debug>
void instr_lwr(N64 &n64, const Opcode &opcode)
{
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);
    u64 addr = (n64.cpu.regs[base] + imm);

    // right, so here 3 is identity
    const u32 offset = 3 - (addr & 3);
    const u32 mask = u32(0xffff'ffff) >> (offset * 8);

    // 'rotate' like an unaligned arm load
    u32 v = read_u32<debug>(n64,addr) >> (offset * 8); 

    // combine reg with load
    v = (v & mask) | (u32(n64.cpu.regs[opcode.rt]) & ~mask);

    // sign extend ans back out;
    n64.cpu.regs[opcode.rt] = sign_extend_mips<s64,s32>(v);
}

template<const b32 debug>
void instr_sw(N64 &n64, const Opcode &opcode)
{
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);

    write_u32<debug>(n64,n64.cpu.regs[base] + imm,n64.cpu.regs[opcode.rt]);
}


template<const b32 debug>
void instr_swl(N64 &n64, const Opcode &opcode)
{
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);
    u64 addr = (n64.cpu.regs[base] + imm);

    const u32 offset = (addr & 3);
    const u32 mask = u32(0xffff'ffff) >> (offset * 8);

    // 'rotate' like an unaligned arm load
    u32 v = u32(n64.cpu.regs[opcode.rt]) >> (offset * 8);

    // combine reg with load
    v = (read_u32<debug>(n64,addr) & ~mask) | (v & mask);

    // sign extend ans back out;
    write_u32<debug>(n64,addr,v);
}

template<const b32 debug>
void instr_swr(N64 &n64, const Opcode &opcode)
{
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);
    u64 addr = (n64.cpu.regs[base] + imm);

    const u32 offset = 3 - (addr & 3);
    const u32 mask = u32(0xffff'ffff) << (offset * 8);

    // 'rotate' like an unaligned arm load
    u32 v = u32(n64.cpu.regs[opcode.rt]) << (offset * 8);

    // combine reg with load
    v = (read_u32<debug>(n64,addr) & ~mask) | (v & mask);

    // sign extend ans back out;
    write_u32<debug>(n64,addr,v);
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
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);
    u64 addr = (n64.cpu.regs[base] + imm);

    const u64 offset = (addr & 7);
    const u64 mask = u64(0xffff'ffff'ffff'ffff) >> (offset * 8);

    // 'rotate' like an unaligned arm load
    u64 v = n64.cpu.regs[opcode.rt] >> (offset * 8);

    // combine reg with load
    v = (read_u64<debug>(n64,addr) & ~mask) | (v & mask);

    // sign extend ans back out;
    write_u64<debug>(n64,addr,v);
}

template<const b32 debug>
void instr_sdr(N64 &n64, const Opcode &opcode)
{
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);
    u64 addr = (n64.cpu.regs[base] + imm);

    const u32 offset = 7 - (addr & 7);
    const u32 mask = u64(0xffff'ffff'ffff'ffff) << (offset * 8);

    // 'rotate' like an unaligned arm load
    u64 v = n64.cpu.regs[opcode.rt] << (offset * 8);

    // combine reg with load
    v = (read_u64<debug>(n64,addr) & ~mask) | (v & mask);

    // sign extend ans back out;
    write_u64<debug>(n64,addr,v);
}

template<const b32 debug>
void instr_ll(N64 &n64, const Opcode &opcode)
{
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);

    const u64 addr = n64.cpu.regs[base] + imm;
    const u64 paddr = remap_addr(n64,addr);

    n64.cpu.cop0.load_linked = paddr >> 4;
    n64.cpu.cop0.ll_bit = true;

    n64.cpu.regs[opcode.rt] = sign_extend_mips<s64,s32>(read_u32<debug>(n64,addr));
}

template<const b32 debug>
void instr_lld(N64 &n64, const Opcode &opcode)
{
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);

    const u64 addr = n64.cpu.regs[base] + imm;
    const u64 paddr = remap_addr(n64,addr);

    n64.cpu.cop0.load_linked = paddr >> 4;
    n64.cpu.cop0.ll_bit = true;

    n64.cpu.regs[opcode.rt] = read_u64<debug>(n64,addr);
}

template<const b32 debug>
void instr_ldr(N64 &n64, const Opcode &opcode)
{
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);
    u64 addr = (n64.cpu.regs[base] + imm);

    // right, so here 3 is identity
    const u32 offset = 7 - (addr & 7);
    const u64 mask = u64(0xffff'ffff'ffff'ffff) >> (offset * 8);

    // 'rotate' like an unaligned arm load
    u64 v = read_u64<debug>(n64,addr) >> (offset * 8); 

    // combine reg with load
    v = (v & mask) | (u64(n64.cpu.regs[opcode.rt]) & ~mask);

    // sign extend ans back out;
    n64.cpu.regs[opcode.rt] = v;
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
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);

    n64.cpu.regs[opcode.rt] = sign_extend_mips<s64,s16>(read_u16<debug>(n64,n64.cpu.regs[base] + imm));
}


void instr_bgtz(N64 &n64, const Opcode &opcode)
{
    instr_branch(n64,opcode,[](N64& n64, const Opcode& opcode)
    {
        return s64(n64.cpu.regs[opcode.rs]) > 0;
    });
}

void instr_blez(N64& n64, const Opcode& opcode)
{
    instr_branch(n64,opcode,[](N64& n64, const Opcode& opcode)
    {
        return s64(n64.cpu.regs[opcode.rs]) <= 0;
    });
}

void instr_bgtzl(N64& n64, const Opcode& opcode)
{
    instr_branch_likely(n64,opcode,[](N64& n64, const Opcode& opcode)
    {
        return s64(n64.cpu.regs[opcode.rs]) > 0;
    }); 
}

}