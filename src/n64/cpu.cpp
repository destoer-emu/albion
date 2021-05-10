#include <n64/n64.h>

namespace nintendo64
{

void reset_cpu(Cpu &cpu)
{
    // setup regs to hle the pif rom
    memset(cpu.regs,0,sizeof(cpu.regs));
    cpu.regs[T3] = 0xFFFFFFFFA4000040;
    cpu.regs[S4] = 0x0000000000000001;
    cpu.regs[S6] = 0x000000000000003F;
    cpu.regs[SP] = 0xFFFFFFFFA4001FF0;

    cpu.cp0_regs[RANDOM] = 0x0000001F;
    cpu.cp0_regs[STATUS] = 0x70400004;
    cpu.cp0_regs[PRID] = 0x00000B00;
    cpu.cp0_regs[CONFIG] = 0x0006E463;

    cpu.pc = 0xA4000040; 
}

void unknown_opcode(N64 &n64, u32 opcode)
{
    const auto err = fmt::format("[cpu {:16x}] unknown opcode {:08x}\n",n64.cpu.pc,opcode);
    n64.debug.trace.print();
    throw std::runtime_error(err);    
}

void step(N64 &n64)
{
    auto &pc = n64.cpu.pc;

    const u32 opcode = read_u32(n64,pc);
    pc += 4;

    // TODO: after we figure out how to decode opcodes again
    // we need to start hooking our debugging tools up
    unknown_opcode(n64,opcode);
}

}