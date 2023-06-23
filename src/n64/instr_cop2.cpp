#include <n64/n64.h>

namespace nintendo64
{
    
void instr_unknown_cop2(N64 &n64, const Opcode &opcode)
{
    const auto err = std::format("[cpu {:16x} {}] unknown cop2 opcode {:08x}\n",n64.cpu.pc,disass_n64(n64,opcode,n64.cpu.pc),(opcode.op >> 21) & 0b11111);
    n64.debug.trace.print();
    throw std::runtime_error(err);    
}

template<const b32 debug>
void instr_COP2(N64 &n64, const Opcode &opcode)
{
    UNUSED(n64); UNUSED(opcode);

    assert(false);
}

template<const b32 debug>
void instr_lwc2(N64 &n64, const Opcode &opcode)
{
    instr_unknown_opcode(n64,opcode);
}

template<const b32 debug>
void instr_ldc2(N64 &n64, const Opcode &opcode)
{
    instr_unknown_opcode(n64,opcode);
}

template<const b32 debug>
void instr_swc2(N64 &n64, const Opcode &opcode)
{
    instr_unknown_opcode(n64,opcode);
}

template<const b32 debug>
void instr_sdc2(N64 &n64, const Opcode &opcode)
{
    instr_unknown_opcode(n64,opcode);
}

}