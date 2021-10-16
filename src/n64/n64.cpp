#include <n64/n64.h>
#include <n64/mips_lut.h>


// unity build
#include <n64/mips.cpp>
#include <n64/mem.cpp>
#include <n64/debug.cpp>
#include <n64/disass.cpp>
#include <n64/cpu.cpp>
#include <n64/instr.cpp>
#include <n64/instr_r.cpp>
#include <n64/instr_regimm.cpp>
#include <n64/instr_cop0.cpp>
#include <n64/mips_lut.cpp>

namespace nintendo64
{

void reset(N64 &n64, const std::string &filename)
{
    reset_mem(n64.mem,filename);
    reset_cpu(n64.cpu);
}

void run(N64 &n64)
{
    // dont know how our vblank setup works
    for(;;)
    {
        step(n64);
    }
}

}