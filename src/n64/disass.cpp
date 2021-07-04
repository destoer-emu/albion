#include <n64/n64.h>

namespace nintendo64
{


std::string disass_unknown(N64 &n64, u32 opcode)
{
    UNUSED(n64);

    // make this panic for now
    printf("disass unknown opcode: %08x\n",opcode);
    exit(1);

    return "unknown opcode";
}


std::string disass_unknown_cop0(N64 &n64, u32 opcode)
{
    UNUSED(n64);

    // make this panic for now
    printf("disass cop0 unknown opcode: %08x\n",opcode);
    exit(1);

    return "unknown cop0 opcode";
}


std::string disass_cop0(N64 &n64, u32 opcode)
{
    UNUSED(n64);

    return disass_cop0_lut[(opcode >> 21) & 0b11111](n64,opcode);
}

std::string disass_mtc0(N64 &n64, u32 opcode)
{
    UNUSED(n64);

    const auto rt = get_rt(opcode);
    const auto rd = get_rd(opcode);

    return fmt::format("mtc0 {}, {}",reg_names[rt],cp0_names[rd]);
}

// TODO: im not sure we need to pass the n64 struct through

// okay lets figure out a good way to decode these again
// and actually worry about the speed of it this time
std::string disass_opcode(N64 &n64,u32 opcode)
{
    return disass_lut[opcode >> 26](n64,opcode);
}

}