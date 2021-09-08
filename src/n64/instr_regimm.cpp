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

}