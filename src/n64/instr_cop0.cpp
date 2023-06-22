#include <n64/n64.h>

namespace nintendo64
{

void instr_unknown_cop0(N64 &n64, const Opcode &opcode)
{
    const auto err = std::format("[cpu {:16x} {}] unknown cop0 opcode {:08x}\n",n64.cpu.pc,disass_opcode(opcode,n64.cpu.pc),(opcode.op >> 21) & 0b11111);
    n64.debug.trace.print();
    throw std::runtime_error(err);    
}

void instr_cop0(N64 &n64, const Opcode &opcode)
{
    instr_cop0_lut[(opcode.op >> 21) & 0b11111](n64,opcode);
}

void instr_mtc0(N64 &n64, const Opcode &opcode)
{
    write_cp0(n64,n64.cpu.regs[opcode.rt],opcode.rd); 
}

}