#include <n64/n64.h>

namespace nintendo64
{

void instr_unknown_cop0(N64 &n64, const Opcode &opcode)
{
    const auto err = std::format("[cpu {:16x} {}] unknown cop0 opcode {:08x}\n",n64.cpu.pc,disass_n64(n64,opcode,n64.cpu.pc),(opcode.op >> 21) & 0b11111);
    n64.debug.trace.print();
    throw std::runtime_error(err);    
}

template<const b32 debug>
void instr_COP0(N64 &n64, const Opcode &opcode)
{
    const u32 offset = calc_cop0_table_offset(opcode);

    if constexpr(debug)
    {
        INSTR_TABLE_DEBUG[offset](n64,opcode);
    }

    else
    {
        INSTR_TABLE_NO_DEBUG[offset](n64,opcode);
    }
}

void instr_mtc0(N64 &n64, const Opcode &opcode)
{
    write_cop0(n64,n64.cpu.regs[opcode.rt],opcode.rd); 
}

void instr_mfc0(N64 &n64, const Opcode &opcode)
{
    n64.cpu.regs[opcode.rt] = read_cop0(n64,opcode.rd); 
}

template<const b32 debug>
void instr_sc(N64 &n64, const Opcode &opcode)
{
    instr_unknown_opcode(n64,opcode);
}

template<const b32 debug>
void instr_scd(N64 &n64, const Opcode &opcode)
{
    instr_unknown_opcode(n64,opcode);
}

}