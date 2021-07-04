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



void cycle_tick(N64 &n64, u32 cycles)
{
    // increment counter (we will shift this when reading)
    // because it should be on every other cycle
    n64.cpu.cp0_regs[COUNT] += cycles;
}


void instr_unknown(N64 &n64, u32 opcode)
{
    const auto err = fmt::format("[cpu {:16x} {}] unknown opcode {:08x}\n",n64.cpu.pc,disass_opcode(n64,opcode),opcode);
    n64.debug.trace.print();
    throw std::runtime_error(err);    
}

void instr_unknown_cop0(N64 &n64, u32 opcode)
{
    const auto err = fmt::format("[cpu {:16x} {}] unknown cop0 opcode {:08x}\n",n64.cpu.pc,disass_opcode(n64,opcode),(opcode >> 21) & 0b11111);
    n64.debug.trace.print();
    throw std::runtime_error(err);    
}


void write_cp0(N64 &n64, u64 v, u32 reg)
{
    auto &regs = n64.cpu.cp0_regs;

    switch(reg)
    {
        // cycle counter (incremented every other cycle)
        case COUNT:
        {
            regs[COUNT] = v;
            break;
        }

        // when this is == count trigger in interrupt
        case COMPARE:
        {
            regs[COMPARE] = v;

            // ip7 in cause is reset when this is written
            regs[CAUSE] = deset_bit(regs[CAUSE],7);
            break;
        }

        // interrupt / exception info
        case CAUSE:
        {
            const auto PENDING_MASK = 0b11 << 8;

            // write can only modify lower 2 bits of interrupt pending
            regs[CAUSE] = (regs[CAUSE] & ~PENDING_MASK)  | (v & PENDING_MASK);
            break;
        }

        default:
        {
            printf("unimplemnted cop0 write: %d\n",reg);
            exit(1);
        }
    }
}

// coprocessor zero insruction
void instr_cop0(N64 &n64, u32 opcode)
{
    instr_cop0_lut[(opcode >> 21) & 0b11111](n64,opcode);
}

void instr_mtc0(N64 &n64, u32 opcode)
{
    const auto rt = get_rt(opcode);
    const auto rd = get_rd(opcode);

    write_cp0(n64,n64.cpu.regs[rt],rd); 
}


void step(N64 &n64)
{
    auto &pc = n64.cpu.pc;

    const u32 opcode = read_u32(n64,pc);

    // TODO: handle the branch delay slot
    pc += 4;

    std::cout << fmt::format("{:16x}: {}\n",pc,disass_opcode(n64,opcode));

    instr_lut[opcode >> 26](n64,opcode);

    // $zero is hardwired to zero, make sure writes cant touch it
    n64.cpu.regs[0] = 0;

    // assume 1 CPI
    cycle_tick(n64,1);

    // check for count interrupt
    // TODO: push this onto the scheduler later
    if((n64.cpu.cp0_regs[COUNT] >> 1) == n64.cpu.cp0_regs[COMPARE])
    {
        puts("counter interrupt!");
        exit(1);
    }
}


}