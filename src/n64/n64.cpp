#include <n64/n64.h>
#include <n64/mips_lut.h>


// unity build
#include <n64/mips.cpp>
#include <n64/mem.cpp>
#include <n64/disass.cpp>
#include <n64/cpu.cpp>
#include <n64/instr.cpp>
#include <n64/instr_r.cpp>
#include <n64/instr_regimm.cpp>
#include <n64/instr_cop0.cpp>
#include <n64/mips_lut.cpp>
#include <n64/rdp.cpp>
#include <n64/debug.cpp>

namespace nintendo64
{

void reset(N64 &n64, const std::string &filename)
{
    reset_mem(n64.mem,filename);
    reset_cpu(n64.cpu);
    reset_rdp(n64.rdp,320,240);
    n64.cycles = 0;
    n64.size_change = false;
}

void run(N64 &n64)
{
    // dont know how our vblank setup works
    while(n64.cycles <= (N64_CLOCK_CYCLES / 60))
    {

#ifdef DEBUG
        if(n64.debug.is_halted())
        {
            return;
        }
#endif
        step(n64);
    }

    // dont know when the rendering should be finished just do at end for now
    render(n64);
    n64.cycles -= N64_CLOCK_CYCLES;
}

}