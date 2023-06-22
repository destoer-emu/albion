#include <n64/n64.h>

namespace nintendo64
{


std::string disass_unknown(const Opcode &opcode, u64 pc)
{
    // make this panic for now
    printf("[%zx]disass unknown opcode: %08x\n",pc-0x4,opcode.op);
    exit(1);

    return "unknown opcode";
}

std::string disass_unknown_regimm(const Opcode &opcode, u64 pc)
{
    // make this panic for now
    printf("[%zx]disass regimm unknown opcode: %08x\n",pc-0x4,opcode.op);
    exit(1);

    return "unknown opcode";    
}


std::string disass_unknown_cop0(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    // make this panic for now
    printf("disass cop0 unknown opcode: %08x\n",opcode.op);
    exit(1);

    return "unknown cop0 opcode";
}


std::string disass_unknown_r(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    // make this panic for now
    printf("disass r unknown opcode: %08x\n",opcode.op);
    exit(1);

    return "unknown r opcode";
}

std::string disass_sync(const Opcode &opcode, u64 pc)
{
    UNUSED(pc); UNUSED(opcode);

    return "sync";
}

std::string disass_lui(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    return std::format("lui {}, {:04x}",reg_names[opcode.rt],opcode.imm);
}

std::string disass_addiu(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);
    return std::format("addiu {}, {}, {:04x}",reg_names[opcode.rt],reg_names[opcode.rs],opcode.imm);
}


std::string disass_addi(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);
    return std::format("addi {}, {}, {:04x}",reg_names[opcode.rt],reg_names[opcode.rs],opcode.imm);
}

std::string disass_daddi(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);
    return std::format("daddi {}, {}, {:04x}",reg_names[opcode.rt],reg_names[opcode.rs],opcode.imm);
}

std::string disass_daddiu(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);
    return std::format("daddiu {}, {}, {:04x}",reg_names[opcode.rt],reg_names[opcode.rs],opcode.imm);
}


std::string disass_ori(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);
    return std::format("ori {}, {}, {:04x}",reg_names[opcode.rt],reg_names[opcode.rs],opcode.imm);
}

std::string disass_andi(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);
    return std::format("andi {}, {}, {:04x}",reg_names[opcode.rt],reg_names[opcode.rs],opcode.imm);
}

std::string disass_xori(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);
    return std::format("xori {}, {}, {:04x}",reg_names[opcode.rt],reg_names[opcode.rs],opcode.imm);
}


std::string disass_slti(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);
    return std::format("slti {}, {}, {:04x}",reg_names[opcode.rt],reg_names[opcode.rs],opcode.imm);
}   

std::string disass_sltiu(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);
    return std::format("sltiu {}, {}, {:04x}",reg_names[opcode.rt],reg_names[opcode.rs],opcode.imm);
}   



std::string disass_jal(const Opcode &opcode, u64 pc)
{
    const auto target = get_target(opcode.op,pc);

    return std::format("jal {:8x}",target);
}

std::string disass_j(const Opcode &opcode, u64 pc)
{
    const auto target = get_target(opcode.op,pc);

    return std::format("j {:8x}",target);
}



std::string disass_lw(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);
    const auto base = opcode.rs;
    return std::format("lw {}, {:04x}({})",reg_names[opcode.rt],opcode.imm,reg_names[base]);
}

std::string disass_lhu(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);
    const auto base = opcode.rs;
    return std::format("lhu {}, {:04x}({})",reg_names[opcode.rt],opcode.imm,reg_names[base]);
}

std::string disass_lb(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);
    const auto base = opcode.rs;
    return std::format("lb {}, {:04x}({})",reg_names[opcode.rt],opcode.imm,reg_names[base]);
}

std::string disass_ld(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);
    const auto base = opcode.rs;
    return std::format("ld {}, {:04x}({})",reg_names[opcode.rt],opcode.imm,reg_names[base]);
}

std::string disass_lwu(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);
    const auto base = opcode.rs;
    return std::format("lwu {}, {:04x}({})",reg_names[opcode.rt],opcode.imm,reg_names[base]);
}


std::string disass_lbu(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    const auto base = opcode.rs;
    return std::format("lbu {}, {:04x}({})",reg_names[opcode.rt],opcode.imm,reg_names[base]);
}


std::string disass_sw(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    const auto base = opcode.rs;
    return std::format("sw {}, {:04x}({})",reg_names[opcode.rt],opcode.imm,reg_names[base]);
}

std::string disass_sh(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    const auto base = opcode.rs;
    return std::format("sh {}, {:04x}({})",reg_names[opcode.rt],opcode.imm,reg_names[base]);
}

std::string disass_sd(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    const auto base = opcode.rs;
    return std::format("sd {}, {:04x}({})",reg_names[opcode.rt],opcode.imm,reg_names[base]);
}

std::string disass_sb(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    const auto base = opcode.rs;
    return std::format("sb {}, {:04x}({})",reg_names[opcode.rt],opcode.imm,reg_names[base]);
}


std::string disass_bne(const Opcode &opcode, u64 pc)
{
    const auto addr = compute_branch_addr(pc,opcode.imm);

    return std::format("bne {}, {}, {:x}",reg_names[opcode.rs],reg_names[opcode.rt],addr);
}

std::string disass_beq(const Opcode &opcode, u64 pc)
{
    const auto addr = compute_branch_addr(pc,opcode.imm);

    if(opcode.rs == 0 && opcode.rt == 0)
    {
        return std::format("b {:x}",addr);
    }

    else
    {
        return std::format("beq {}, {}, {:x}",reg_names[opcode.rs],reg_names[opcode.rt],addr);
    }
}

std::string disass_beql(const Opcode &opcode, u64 pc)
{
    const auto addr = compute_branch_addr(pc,opcode.imm);

    return std::format("beql {}, {}, {:x}",reg_names[opcode.rs],reg_names[opcode.rt],addr);
}

std::string disass_blezl(const Opcode &opcode, u64 pc)
{
    const auto addr = compute_branch_addr(pc,opcode.imm);

    return std::format("blezl {}, {:x}",reg_names[opcode.rs],addr);
}

std::string disass_bgezl(const Opcode &opcode, u64 pc)
{
    const auto addr = compute_branch_addr(pc,opcode.imm);

    return std::format("bgezl {}, {:x}",reg_names[opcode.rs],addr);
}

std::string disass_bgtz(const Opcode &opcode, u64 pc)
{
    const auto addr = compute_branch_addr(pc,opcode.imm);

    return std::format("bgtz {}, {:x}",reg_names[opcode.rs],addr);
}

std::string disass_bgezal(const Opcode &opcode, u64 pc)
{
    const auto addr = compute_branch_addr(pc,opcode.imm);

    if(opcode.rs == 0)
    {
        return std::format("bal {}, {:x}",reg_names[opcode.rs],addr);
    }

    else
    {
        return std::format("bgezl {}, {:x}",reg_names[opcode.rs],addr);
    }
}


std::string disass_bnel(const Opcode &opcode, u64 pc)
{
    const auto addr = compute_branch_addr(pc,opcode.imm);

    return std::format("bnel {}, {}, {:x}",reg_names[opcode.rs],reg_names[opcode.rt],addr);
}

std::string disass_cache(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    const auto base = opcode.rs;
    const auto op = opcode.rt;

    return std::format("cache {}, {:04x}({})",op,opcode.imm,reg_names[base]);
}

std::string disass_subu(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);
    return std::format("subu {}, {}, {}",reg_names[opcode.rd],reg_names[opcode.rs],reg_names[opcode.rt]);
}

std::string disass_and(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    return std::format("and {}, {}, {}",reg_names[opcode.rd],reg_names[opcode.rs],reg_names[opcode.rt]);
}

std::string disass_addu(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    return std::format("addu {}, {}, {}",reg_names[opcode.rd],reg_names[opcode.rs],reg_names[opcode.rt]);
}

std::string disass_add(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    return std::format("add {}, {}, {}",reg_names[opcode.rd],reg_names[opcode.rs],reg_names[opcode.rt]);
}

std::string disass_multu(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    return std::format("multu {}, {}",reg_names[opcode.rs],reg_names[opcode.rt]);
}

std::string disass_mflo(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    return std::format("mflo {}",reg_names[opcode.rd]);
}


std::string disass_sltu(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    return std::format("sltu {}, {}, {}",reg_names[opcode.rd],reg_names[opcode.rs],reg_names[opcode.rt]);    
}

std::string disass_slt(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    return std::format("slt {}, {}, {}",reg_names[opcode.rd],reg_names[opcode.rs],reg_names[opcode.rt]);    
}


std::string disass_mtc0(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    return std::format("mtc0 {}, {}",reg_names[opcode.rt],cp0_names[opcode.rd]);
}

std::string disass_sll(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    if(opcode.op)
    {
        const auto shamt = get_shamt(opcode.op);
        
        return std::format("sll {}, {}, {}",reg_names[opcode.rd],reg_names[opcode.rt],shamt);
    }

    else
    {
        return "nop";
    }
}

std::string disass_dsll(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    const auto shamt = get_shamt(opcode.op);
    
    return std::format("dsll {}, {}, {}",reg_names[opcode.rd],reg_names[opcode.rt],shamt);   
}

std::string disass_dsll32(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    const auto shamt = get_shamt(opcode.op);
    
    return std::format("dsll32 {}, {}, {}",reg_names[opcode.rd],reg_names[opcode.rt],shamt);   
}


std::string disass_jalr(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    return std::format("jalr {}, {}",reg_names[opcode.rd],reg_names[opcode.rs]);
}

std::string disass_sllv(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    return std::format("sllv {}, {}, {}",reg_names[opcode.rd],reg_names[opcode.rt],reg_names[opcode.rs]);
}

std::string disass_dsllv(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    return std::format("dsllv {}, {}, {}",reg_names[opcode.rd],reg_names[opcode.rt],reg_names[opcode.rs]);
}


std::string disass_srl(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    const auto shamt = get_shamt(opcode.op);

    return std::format("srl {}, {}, {}",reg_names[opcode.rd],reg_names[opcode.rt],shamt);
}

std::string disass_sra(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    const auto shamt = get_shamt(opcode.op);

    return std::format("sra {}, {}, {}",reg_names[opcode.rd],reg_names[opcode.rt],shamt);
}


std::string disass_dsra32(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    const auto shamt = get_shamt(opcode.op);

    return std::format("dsra32 {}, {}, {}",reg_names[opcode.rd],reg_names[opcode.rt],shamt);
}

std::string disass_srlv(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    return std::format("srlv {}, {}, {}",reg_names[opcode.rd],reg_names[opcode.rt],reg_names[opcode.rs]);
}


std::string disass_or(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    return std::format("or {}, {}, {}",reg_names[opcode.rd],reg_names[opcode.rs],reg_names[opcode.rt]);    
}


std::string disass_nor(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    return std::format("nor {}, {}, {}",reg_names[opcode.rd],reg_names[opcode.rs],reg_names[opcode.rt]);    
}
std::string disass_xor(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    return std::format("xor {}, {}, {}",reg_names[opcode.rd],reg_names[opcode.rs],reg_names[opcode.rt]);    
}



std::string disass_jr(const Opcode &opcode, u64 pc)
{
    UNUSED(pc);

    return std::format("jr {}",reg_names[opcode.rs]);
}

std::string disass_cop0(const Opcode &opcode, u64 pc)
{
    return disass_cop0_lut[(opcode.op >> 21) & 0b11111](opcode,pc);
}

std::string disass_regimm(const Opcode &opcode, u64 pc)
{
    return disass_regimm_lut[(opcode.op >> 16) & 0b11111](opcode,pc);
}


std::string disass_r_fmt(const Opcode &opcode, u64 pc)
{
    return disass_r_lut[opcode.op & 0b111111](opcode,pc);
}

// okay lets figure out a good way to decode these again
// and actually worry about the speed of it this time
std::string disass_opcode(const Opcode &opcode, u64 pc)
{
    return disass_lut[opcode.op >> 26](opcode,pc);
}

}