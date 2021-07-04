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
    write_cp0(cpu,0x70400004,STATUS);
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


void write_cp0(Cpu &cpu, u64 v, u32 reg)
{
    auto &regs = cpu.cp0_regs;

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

        // various cpu settings
        case STATUS:
        {
            cpu.ie = is_set(v,0);
            cpu.exl = is_set(v,1);
            cpu.erl = is_set(v,2);
            cpu.ksu = (v >> 3) & 0b11;

            cpu.ux = is_set(v,5);
            cpu.sx = is_set(v,6);
            cpu.kx = is_set(v,7);

            cpu.im = (v >> 8) & 0xff;

            cpu.ds = (v >> 16) & 0x1ff;

            cpu.re = is_set(v,25);
            cpu.fr = is_set(v,26);
            cpu.rp = is_set(v,27);

            cpu.cu1 = is_set(v,29);


            if(cpu.rp)
            {
                unimplemented("low power mode");
            }

            if(cpu.re)
            {
                unimplemented("little endian");
            }

            if((cpu.ux && cpu.ksu == 0b10) || (cpu.sx && cpu.ksu == 0b01) || (cpu.kx && cpu.ksu == 0b00))
            {
                unimplemented("64 bit addressing");
            }


            // make this easier to read back out to the cpu
            regs[STATUS] = v;
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
            printf("unimplemented cop0 write: %d\n",reg);
            exit(1);
        }
    }
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
        // flag interrupt
        n64.cpu.cp0_regs[CAUSE] = set_bit(n64.cpu.cp0_regs[CAUSE],15);
    }
}


}