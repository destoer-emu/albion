#include <n64/n64.h>

namespace nintendo64
{

void instr_unknown_r(N64 &n64, const Opcode &opcode)
{
    const auto err = fmt::format("[cpu {:16x} {}] unknown r opcode {:08x}\n",n64.cpu.pc,disass_n64(n64,opcode,n64.cpu.pc),opcode.op & 0b11111);
    n64.debug.trace.print();
    throw std::runtime_error(err);        
}

template<const b32 debug>
void instr_SPECIAL(N64& n64, const Opcode& opcode)
{
    if constexpr(debug)
    {
        INSTR_TABLE_DEBUG[SPECIAL_OFFSET + ((opcode.op >> SPECIAL_SHIFT) & FUNCT_MASK)](n64,opcode);
    }

    else
    {
        INSTR_TABLE_NO_DEBUG[SPECIAL_OFFSET + ((opcode.op >> SPECIAL_SHIFT) & FUNCT_MASK)](n64,opcode);
    }
}

void instr_sll(N64 &n64, const Opcode &opcode)
{
    const auto shamt = get_shamt(opcode.op);

    n64.cpu.regs[opcode.rd] = sign_extend_mips<s64,s32>(u32(n64.cpu.regs[opcode.rt]) << shamt);
}

void instr_dsll(N64 &n64, const Opcode &opcode)
{
    const auto shamt = get_shamt(opcode.op);

    n64.cpu.regs[opcode.rd] = n64.cpu.regs[opcode.rt] << shamt;
}

void instr_dsll32(N64 &n64, const Opcode &opcode)
{
    const auto shamt = get_shamt(opcode.op);

    n64.cpu.regs[opcode.rd] = n64.cpu.regs[opcode.rt] << u64(shamt + 32);
}



void instr_sllv(N64 &n64, const Opcode &opcode)
{
    n64.cpu.regs[opcode.rd] = sign_extend_mips<s64,s32>(u32(n64.cpu.regs[opcode.rt]) << (n64.cpu.regs[opcode.rs] & 0b11111));    
}

void instr_dsllv(N64 &n64, const Opcode &opcode)
{
    n64.cpu.regs[opcode.rd] = n64.cpu.regs[opcode.rt] << u64(n64.cpu.regs[opcode.rs] & 0b111111);    
}

void instr_sra(N64 &n64, const Opcode &opcode)
{
    const auto shamt = get_shamt(opcode.op);

    // shift, clamp, sign extend
    // thanks dillon
    const auto v = s32(s64(n64.cpu.regs[opcode.rt]) >> shamt);
    n64.cpu.regs[opcode.rd] = sign_extend_mips<s64,s32>(v);
}

void instr_srav(N64 &n64, const Opcode &opcode)
{
    // shift, clamp, sign extend
    // thanks dillon
    const auto v = s32(s64(n64.cpu.regs[opcode.rt]) >> (n64.cpu.regs[opcode.rs] & 0b111'11));
    n64.cpu.regs[opcode.rd] = sign_extend_mips<s64,s32>(v);
}

void instr_dsrav(N64 &n64, const Opcode &opcode)
{
    instr_unknown_r(n64,opcode);
}

void instr_syscall(N64 &n64, const Opcode &opcode)
{
    instr_unknown_r(n64,opcode);
}

void instr_mfhi(N64 &n64, const Opcode &opcode)
{
    n64.cpu.regs[opcode.rd] = n64.cpu.hi;
}

void instr_mthi(N64 &n64, const Opcode &opcode)
{
    n64.cpu.hi = n64.cpu.regs[opcode.rs];
}

void instr_mtlo(N64 &n64, const Opcode &opcode)
{
    n64.cpu.lo = n64.cpu.regs[opcode.rs];
}

void instr_srl(N64 &n64, const Opcode &opcode)
{
    const auto shamt = get_shamt(opcode.op);

    n64.cpu.regs[opcode.rd] = sign_extend_mips<s64,s32>(u32(n64.cpu.regs[opcode.rt]) >> shamt);    
}

void instr_dsrl(N64 &n64, const Opcode &opcode)
{
    instr_unknown_r(n64,opcode);
}

void instr_dsrl32(N64 &n64, const Opcode &opcode)
{
    instr_unknown_r(n64,opcode);
}

void instr_dsra(N64 &n64, const Opcode &opcode)
{
    instr_unknown_r(n64,opcode);
}

void instr_dsra32(N64 &n64, const Opcode &opcode)
{
    const auto shamt = get_shamt(opcode.op);

    n64.cpu.regs[opcode.rd] = s64(n64.cpu.regs[opcode.rt]) >> u64(shamt + 32);    
}


void instr_srlv(N64 &n64, const Opcode &opcode)
{
    n64.cpu.regs[opcode.rd] = sign_extend_mips<s64,s32>(u32(n64.cpu.regs[opcode.rt]) >> (n64.cpu.regs[opcode.rs] & 0b11111));    
}

void instr_dsrlv(N64 &n64, const Opcode &opcode)
{
    instr_unknown_r(n64,opcode);
}

void instr_sltu(N64 &n64, const Opcode &opcode)
{
    n64.cpu.regs[opcode.rd] = n64.cpu.regs[opcode.rs] < n64.cpu.regs[opcode.rt];    
}

void instr_slt(N64 &n64, const Opcode &opcode)
{
    n64.cpu.regs[opcode.rd] = s64(n64.cpu.regs[opcode.rs]) < s64(n64.cpu.regs[opcode.rt]);    
}

void instr_sub(N64& n64, const Opcode& opcode)
{
    const u64 ans = sign_extend_mips<s64,s32>(u32(n64.cpu.regs[opcode.rs]) - u32(n64.cpu.regs[opcode.rt]));

    if(ssub_overflow(s32(n64.cpu.regs[opcode.rs]),s32(n64.cpu.regs[opcode.rt])))
    {
        unimplemented("sub overflow");
    }

    else
    {
        n64.cpu.regs[opcode.rd] = ans;
    }
}

void instr_subu(N64 &n64, const Opcode &opcode)
{
    // does not trap on overflow
    n64.cpu.regs[opcode.rd] = sign_extend_mips<s64,s32>(u32(n64.cpu.regs[opcode.rs]) - u32(n64.cpu.regs[opcode.rt]));
}

void instr_dsub(N64 &n64, const Opcode &opcode)
{
    const u64 ans = n64.cpu.regs[opcode.rs] - n64.cpu.regs[opcode.rt];

    if(ssub_overflow(s64(n64.cpu.regs[opcode.rs]),s64(n64.cpu.regs[opcode.rt])))
    {
        unimplemented("sub overflow");
    }

    else
    {
        n64.cpu.regs[opcode.rd] = ans;
    }
}

void instr_dsubu(N64 &n64, const Opcode &opcode)
{
    instr_unknown_r(n64,opcode);
}

void instr_addu(N64 &n64, const Opcode &opcode)
{
    // does not trap on overflow
    n64.cpu.regs[opcode.rd] = sign_extend_mips<s64,s32>(u32(n64.cpu.regs[opcode.rs]) + u32(n64.cpu.regs[opcode.rt]));
}

void instr_add(N64 &n64, const Opcode &opcode)
{
    const auto ans = sign_extend_mips<s64,s32>(u32(n64.cpu.regs[opcode.rs]) + u32(n64.cpu.regs[opcode.rt]));

    if(sadd_overflow(s32(n64.cpu.regs[opcode.rs]),s32(n64.cpu.regs[opcode.rt])))
    {
        unimplemented("instr_add overflow");
    }

    else
    {
        n64.cpu.regs[opcode.rd] = ans;
    }
}

void instr_dadd(N64 &n64, const Opcode &opcode)
{
    const u64 ans = n64.cpu.regs[opcode.rs] + n64.cpu.regs[opcode.rt];

    if(sadd_overflow(s64(n64.cpu.regs[opcode.rs]),s64(n64.cpu.regs[opcode.rt])))
    {
        unimplemented("instr_dadd overflow");
    }

    else
    {
        n64.cpu.regs[opcode.rd] = ans;
    }
}

void instr_daddu(N64 &n64, const Opcode &opcode)
{
    n64.cpu.regs[opcode.rd] = n64.cpu.regs[opcode.rs] + n64.cpu.regs[opcode.rt];
}


void instr_and(N64 &n64, const Opcode &opcode)
{
    n64.cpu.regs[opcode.rd] = n64.cpu.regs[opcode.rs] & n64.cpu.regs[opcode.rt];
}



void instr_multu(N64 &n64, const Opcode &opcode)
{
    const u64 res = u64(u32(n64.cpu.regs[opcode.rs])) * u64(u32(n64.cpu.regs[opcode.rt]));

    n64.cpu.lo = sign_extend_mips<s64,s32>(res & 0xffffffff);
    n64.cpu.hi = sign_extend_mips<s64,s32>((res >> 32) & 0xffffffff);
}

void instr_mult(N64 &n64, const Opcode &opcode)
{
    const u64 res = s64(s32(n64.cpu.regs[opcode.rs])) * s64(s32(n64.cpu.regs[opcode.rt]));

    n64.cpu.lo = sign_extend_mips<s64,s32>(res & 0xffffffff);
    n64.cpu.hi = sign_extend_mips<s64,s32>((res >> 32) & 0xffffffff);
}

void instr_dmultu(N64 &n64, const Opcode &opcode)
{
    instr_unknown_r(n64,opcode);
}

void instr_dmult(N64 &n64, const Opcode &opcode)
{
    instr_unknown_r(n64,opcode);
}

void instr_break(N64 &n64, const Opcode &opcode)
{
    instr_unknown_r(n64,opcode);
}

void instr_div(N64 &n64, const Opcode &opcode)
{
    // div by zero not allowed
    if(n64.cpu.regs[opcode.rt] == 0)
    {
        if(is_set(n64.cpu.regs[opcode.rs],31))
        {
            n64.cpu.lo = 1;
        }

        else
        {
            n64.cpu.lo = 0xffff'ffff;
        }
        n64.cpu.hi = n64.cpu.regs[opcode.rs];
        return;
    }

    // prevent int_min division trap
    const s32 res = s64(n64.cpu.regs[opcode.rs]) / s64(n64.cpu.regs[opcode.rt]);
    const s32 remainder = s64(n64.cpu.regs[opcode.rs]) % s64(n64.cpu.regs[opcode.rt]);

    n64.cpu.lo = sign_extend_mips<s64,s32>(res);
    n64.cpu.hi = sign_extend_mips<s64,s32>(remainder);  
}

void instr_divu(N64 &n64, const Opcode &opcode)
{
    // div by zero not allowed
    if(n64.cpu.regs[opcode.rt] == 0)
    {
        n64.cpu.lo = 0xffff'ffff;
        n64.cpu.hi = n64.cpu.regs[opcode.rs];
        return;
    }

    // prevent int_min division trap
    const u32 res = s64(n64.cpu.regs[opcode.rs]) / s64(n64.cpu.regs[opcode.rt]);
    const u32 remainder = s64(n64.cpu.regs[opcode.rs]) % s64(n64.cpu.regs[opcode.rt]);

    n64.cpu.lo = sign_extend_mips<s64,s32>(res);
    n64.cpu.hi = sign_extend_mips<s64,s32>(remainder);    
}

void instr_ddiv(N64 &n64, const Opcode &opcode)
{
    instr_unknown_r(n64,opcode);
}

void instr_ddivu(N64 &n64, const Opcode &opcode)
{
    instr_unknown_r(n64,opcode);
}

void instr_mflo(N64 &n64, const Opcode &opcode)
{
    n64.cpu.regs[opcode.rd] = n64.cpu.lo;
}

void instr_or(N64 &n64, const Opcode &opcode)
{
    n64.cpu.regs[opcode.rd] = n64.cpu.regs[opcode.rs] | n64.cpu.regs[opcode.rt];
}

void instr_nor(N64 &n64, const Opcode &opcode)
{
    n64.cpu.regs[opcode.rd] = ~(n64.cpu.regs[opcode.rs] | n64.cpu.regs[opcode.rt]);
}

void instr_xor(N64 &n64, const Opcode &opcode)
{
    n64.cpu.regs[opcode.rd] = n64.cpu.regs[opcode.rs] ^ n64.cpu.regs[opcode.rt];
}


void instr_jr(N64 &n64, const Opcode &opcode)
{
    write_pc(n64,n64.cpu.regs[opcode.rs]);
}

void instr_jalr(N64 &n64, const Opcode &opcode)
{
    const auto target = n64.cpu.regs[opcode.rs];

    n64.cpu.regs[opcode.rd] = n64.cpu.pc_next;

    write_pc(n64,target);
}

void instr_sync(N64 &n64, const Opcode &opcode)
{
    // does this do anything?
    UNUSED(n64); UNUSED(opcode); 
}


}