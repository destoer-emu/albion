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
#include <n64/scheduler.cpp>

namespace nintendo64
{

void reset(N64 &n64, const std::string &filename)
{
    reset_mem(n64.mem,filename);
    reset_cpu(n64);
    reset_rdp(n64,320,240);
    n64.size_change = false;
}

void run(N64 &n64)
{
    n64.rdp.frame_done = false;

    // dont know how our vblank setup works
    while(!n64.rdp.frame_done)
    {

        while(!n64.scheduler.event_ready())
        {

#ifdef DEBUG
            if(n64.debug.is_halted())
            {
                return;
            }
#endif
            step(n64);
        }
        n64.scheduler.service_events();
    }

    // dont know when the rendering should be finished just do at end for now
    render(n64);
}

}