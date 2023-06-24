#include <n64/n64.h>

namespace nintendo64
{

void instr_unknown_cop1(N64 &n64, const Opcode &opcode)
{
    const auto err = std::format("[cpu {:16x} {}] unknown cop1 opcode {:08x} : {:08x}\n",
        n64.cpu.pc,disass_n64(n64,opcode,n64.cpu.pc),(opcode.op >> 21) & 0b11111,opcode.op);

    n64.debug.trace.print();
    throw std::runtime_error(err);    
}

template<const b32 debug>
void instr_COP1(N64 &n64, const Opcode &opcode)
{
    UNUSED(n64); UNUSED(opcode);

    instr_unknown_cop1(n64,opcode);
}

template<const b32 debug>
void instr_lwc1(N64 &n64, const Opcode &opcode)
{
    instr_unknown_opcode(n64,opcode);
}

template<const b32 debug>
void instr_ldc1(N64 &n64, const Opcode &opcode)
{
    instr_unknown_opcode(n64,opcode);
}

template<const b32 debug>
void instr_swc1(N64 &n64, const Opcode &opcode)
{
    instr_unknown_opcode(n64,opcode);
}

template<const b32 debug>
void instr_sdc1(N64 &n64, const Opcode &opcode)
{
    instr_unknown_opcode(n64,opcode);
}

}