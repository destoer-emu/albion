#include <n64/n64.h>

namespace nintendo64
{


std::string disass_unknown(u32 opcode, u64 pc)
{
    // make this panic for now
    printf("[%zx]disass unknown opcode: %08x\n",pc-0x4,opcode);
    exit(1);

    return "unknown opcode";
}

std::string disass_unknown_regimm(u32 opcode, u64 pc)
{
    // make this panic for now
    printf("[%zx]disass regimm unknown opcode: %08x\n",pc-0x4,opcode);
    exit(1);

    return "unknown opcode";    
}


std::string disass_unknown_cop0(u32 opcode, u64 pc)
{
    UNUSED(pc);

    // make this panic for now
    printf("disass cop0 unknown opcode: %08x\n",opcode);
    exit(1);

    return "unknown cop0 opcode";
}


std::string disass_unknown_r(u32 opcode, u64 pc)
{
    UNUSED(pc);

    // make this panic for now
    printf("disass r unknown opcode: %08x\n",opcode);
    exit(1);

    return "unknown r opcode";
}



std::string disass_lui(u32 opcode, u64 pc)
{
    UNUSED(pc);

    const auto rt = get_rt(opcode);

    return fmt::format("lui {}, {:04x}",reg_names[rt],opcode & 0xffff);
}

std::string disass_addiu(u32 opcode, u64 pc)
{
    UNUSED(pc);

    const auto rt = get_rt(opcode);
    const auto rs = get_rs(opcode);

    const auto imm = opcode & 0xffff;

    return fmt::format("addiu {}, {}, {:04x}",reg_names[rt],reg_names[rs],imm);
}


std::string disass_addi(u32 opcode, u64 pc)
{
    UNUSED(pc);

    const auto rt = get_rt(opcode);
    const auto rs = get_rs(opcode);

    const auto imm = opcode & 0xffff;

    return fmt::format("addi {}, {}, {:04x}",reg_names[rt],reg_names[rs],imm);
}


std::string disass_ori(u32 opcode, u64 pc)
{
    UNUSED(pc);

    const auto rt = get_rt(opcode);
    const auto rs = get_rs(opcode);

    const auto imm = opcode & 0xffff;

    return fmt::format("ori {}, {}, {:04x}",reg_names[rt],reg_names[rs],imm);
}

std::string disass_andi(u32 opcode, u64 pc)
{
    UNUSED(pc);

    const auto rt = get_rt(opcode);
    const auto rs = get_rs(opcode);

    const auto imm = opcode & 0xffff;

    return fmt::format("andi {}, {}, {:04x}",reg_names[rt],reg_names[rs],imm);
}

std::string disass_xori(u32 opcode, u64 pc)
{
    UNUSED(pc);

    const auto rt = get_rt(opcode);
    const auto rs = get_rs(opcode);

    const auto imm = opcode & 0xffff;

    return fmt::format("xori {}, {}, {:04x}",reg_names[rt],reg_names[rs],imm);
}


std::string disass_slti(u32 opcode, u64 pc)
{
    UNUSED(pc);


    const auto rt = get_rt(opcode);
    const auto rs = get_rs(opcode);

    const auto imm = opcode & 0xffff;

    return fmt::format("slti {}, {}, {:04x}",reg_names[rt],reg_names[rs],imm);
}   


std::string disass_jal(u32 opcode, u64 pc)
{
    const auto target = get_target(opcode,pc);

    return fmt::format("jal {:8x}",target);
}

std::string disass_lw(u32 opcode, u64 pc)
{
    UNUSED(pc);

    const auto base = get_rs(opcode);
    const auto rt = get_rt(opcode);

    const auto imm = opcode & 0xffff;  

    return fmt::format("lw {}, {:04x}({})",reg_names[rt],imm,reg_names[base]);
}

std::string disass_lbu(u32 opcode, u64 pc)
{
    UNUSED(pc);

    const auto base = get_rs(opcode);
    const auto rt = get_rt(opcode);

    const auto imm = opcode & 0xffff;  

    return fmt::format("lbu {}, {:04x}({})",reg_names[rt],imm,reg_names[base]);
}


std::string disass_sw(u32 opcode, u64 pc)
{
    UNUSED(pc);

    const auto base = get_rs(opcode);
    const auto rt = get_rt(opcode);

    const auto imm = opcode & 0xffff;  

    return fmt::format("sw {}, {:04x}({})",reg_names[rt],imm,reg_names[base]);
}

std::string disass_sb(u32 opcode, u64 pc)
{
    UNUSED(pc);

    const auto base = get_rs(opcode);
    const auto rt = get_rt(opcode);

    const auto imm = opcode & 0xffff;  

    return fmt::format("sb {}, {:04x}({})",reg_names[rt],imm,reg_names[base]);
}


std::string disass_bne(u32 opcode, u64 pc)
{
    const auto rs = get_rs(opcode);
    const auto rt = get_rt(opcode);

    const auto imm = opcode & 0xffff;

    const auto addr = compute_branch_addr(pc,imm);

    return fmt::format("bne {}, {}, {:x}",reg_names[rs],reg_names[rt],addr);
}

std::string disass_beq(u32 opcode, u64 pc)
{
    const auto rs = get_rs(opcode);
    const auto rt = get_rt(opcode);

    const auto imm = opcode & 0xffff;

    const auto addr = compute_branch_addr(pc,imm);

    if(rs == 0 && rt == 0)
    {
        return fmt::format("b {:x}",addr);
    }

    else
    {
        return fmt::format("bne {}, {}, {:x}",reg_names[rs],reg_names[rt],addr);
    }
}

std::string disass_beql(u32 opcode, u64 pc)
{
    const auto rs = get_rs(opcode);
    const auto rt = get_rt(opcode);

    const auto imm = opcode & 0xffff;

    const auto addr = compute_branch_addr(pc,imm);

    return fmt::format("beql {}, {}, {:x}",reg_names[rs],reg_names[rt],addr);
}

std::string disass_blezl(u32 opcode, u64 pc)
{
    const auto rs = get_rs(opcode);

    const auto imm = opcode & 0xffff;

    const auto addr = compute_branch_addr(pc,imm);

    return fmt::format("blezl {}, {:x}",reg_names[rs],addr);
}

std::string disass_bgezl(u32 opcode, u64 pc)
{
    const auto rs = get_rs(opcode);

    const auto imm = opcode & 0xffff;

    const auto addr = compute_branch_addr(pc,imm);

    return fmt::format("bgezl {}, {:x}",reg_names[rs],addr);
}


std::string disass_bnel(u32 opcode, u64 pc)
{
    const auto rs = get_rs(opcode);
    const auto rt = get_rt(opcode);

    const auto imm = opcode & 0xffff;

    const auto addr = compute_branch_addr(pc,imm);

    return fmt::format("bnel {}, {}, {:x}",reg_names[rs],reg_names[rt],addr);
}

std::string disass_cache(u32 opcode, u64 pc)
{
    UNUSED(pc);

    const auto base = get_rs(opcode);
    const auto op = get_rt(opcode);

    const auto imm = opcode & 0xffff;

    return fmt::format("cache {}, {:04x}({})",op,imm,reg_names[base]);
}

std::string disass_subu(u32 opcode, u64 pc)
{
    UNUSED(pc);

    const auto rd = get_rd(opcode);
    const auto rt = get_rt(opcode);
    const auto rs = get_rs(opcode);


    return fmt::format("subu {}, {}, {}",reg_names[rd],reg_names[rs],reg_names[rt]);
}

std::string disass_and(u32 opcode, u64 pc)
{
    UNUSED(pc);

    const auto rd = get_rd(opcode);
    const auto rt = get_rt(opcode);
    const auto rs = get_rs(opcode);


    return fmt::format("and {}, {}, {}",reg_names[rd],reg_names[rs],reg_names[rt]);
}

std::string disass_addu(u32 opcode, u64 pc)
{
    UNUSED(pc);

    const auto rd = get_rd(opcode);
    const auto rt = get_rt(opcode);
    const auto rs = get_rs(opcode);


    return fmt::format("addu {}, {}, {}",reg_names[rd],reg_names[rs],reg_names[rt]);
}

std::string disass_multu(u32 opcode, u64 pc)
{
    UNUSED(pc);

    const auto rt = get_rt(opcode);
    const auto rs = get_rs(opcode);


    return fmt::format("multu {}, {}",reg_names[rs],reg_names[rt]);
}

std::string disass_mflo(u32 opcode, u64 pc)
{
    UNUSED(pc);

    const auto rd = get_rd(opcode);

    return fmt::format("mflo {}",reg_names[rd]);
}


std::string disass_sltu(u32 opcode, u64 pc)
{
    UNUSED(pc);

    const auto rt = get_rt(opcode);
    const auto rs = get_rs(opcode);
    const auto rd = get_rd(opcode);

    return fmt::format("sltu {}, {}, {}",reg_names[rd],reg_names[rs],reg_names[rt]);    
}

std::string disass_slt(u32 opcode, u64 pc)
{
    UNUSED(pc);

    const auto rt = get_rt(opcode);
    const auto rs = get_rs(opcode);
    const auto rd = get_rd(opcode);

    return fmt::format("slt {}, {}, {}",reg_names[rd],reg_names[rs],reg_names[rt]);    
}


std::string disass_mtc0(u32 opcode, u64 pc)
{
    UNUSED(pc);

    const auto rt = get_rt(opcode);
    const auto rd = get_rd(opcode);

    return fmt::format("mtc0 {}, {}",reg_names[rt],cp0_names[rd]);
}

std::string disass_sll(u32 opcode, u64 pc)
{
    UNUSED(pc);

    if(opcode)
    {
        const auto rt = get_rt(opcode);
        const auto rd = get_rd(opcode);

        const auto shamt = get_shamt(opcode);
        
        
        return fmt::format("sll {}, {}, {}",reg_names[rd],reg_names[rt],shamt);
    }

    else
    {
        return "nop";
    }
}

std::string disass_srl(u32 opcode, u64 pc)
{
    const auto rt = get_rt(opcode);
    const auto rd = get_rd(opcode);

    const auto shamt = get_shamt(opcode);
    
    
    return fmt::format("srl {}, {}, {}",reg_names[rd],reg_names[rt],shamt);
}

std::string disass_or(u32 opcode, u64 pc)
{
    UNUSED(pc);

    const auto rt = get_rt(opcode);
    const auto rd = get_rd(opcode);
    const auto rs = get_rs(opcode);

    return fmt::format("or {}, {}, {}",reg_names[rd],reg_names[rs],reg_names[rt]);    
}

std::string disass_jr(u32 opcode, u64 pc)
{
    UNUSED(pc);

    const auto rs = get_rs(opcode);

    return fmt::format("jr {}",reg_names[rs]);
}

std::string disass_cop0(u32 opcode, u64 pc)
{
    return disass_cop0_lut[(opcode >> 21) & 0b11111](opcode,pc);
}

std::string disass_regimm(u32 opcode, u64 pc)
{
    return disass_regimm_lut[(opcode >> 16) & 0b11111](opcode,pc);
}


std::string disass_r_fmt(u32 opcode, u64 pc)
{
    return disass_r_lut[opcode & 0b111111](opcode,pc);
}

// okay lets figure out a good way to decode these again
// and actually worry about the speed of it this time
std::string disass_opcode(u32 opcode, u64 pc)
{
    return disass_lut[opcode >> 26](opcode,pc);
}

}