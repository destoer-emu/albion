#include <n64/n64.h>

namespace nintendo64
{


std::string disass_unknown(u32 opcode, u64 pc)
{
    UNUSED(pc);

    // make this panic for now
    printf("[%zx]disass unknown opcode: %08x\n",pc,opcode);
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

std::string disass_lw(u32 opcode, u64 pc)
{
    UNUSED(pc);

    const auto base = get_rs(opcode);
    const auto rt = get_rt(opcode);

    const auto imm = opcode & 0xffff;  

    return fmt::format("lw {}, {:04x}({})",reg_names[rt],imm,reg_names[base]);
}

std::string disass_bne(u32 opcode, u64 pc)
{
    const auto rs = get_rs(opcode);
    const auto rt = get_rt(opcode);

    const auto imm = opcode & 0xffff;

    const auto addr = compute_branch_addr(pc,imm);

    return fmt::format("bne {}, {}, {:x}",reg_names[rs],reg_names[rt],addr);
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

    const auto rt = get_rt(opcode);
    const auto rd = get_rd(opcode);

    const auto shamt = get_shamt(opcode);

    return fmt::format("sll {}, {}, {}",reg_names[rd],reg_names[rt],shamt);
}

std::string disass_cop0(u32 opcode, u64 pc)
{
    return disass_cop0_lut[(opcode >> 21) & 0b11111](opcode,pc);
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