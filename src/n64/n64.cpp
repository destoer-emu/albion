#include <n64/n64.h>
#include <n64/mips_lut.h>

// TODO:
// make dmas not complete instantly, but rather memcpy at the end

// unity build
#include <beyond_all_repair.cpp>
using namespace beyond_all_repair;

#include <n64/mem/mem.cpp>
#include <n64/cpu/cpu.cpp>
#include <n64/mem/layout.cpp>
#include <n64/instr/instr.cpp>
#include <n64/instr/mips_lut.cpp>
#include <n64/rcp/rdp.cpp>
#include <n64/debug.cpp>
#include <n64/scheduler.cpp>

namespace nintendo64
{

b32 read_func(beyond_all_repair::Program& program,u64 addr,void* out, u32 size)
{
    auto& n64 = *(N64*)program.data;

    switch(size)
    {
        case 1:
        {
            const u8 v = read_u8<false>(n64,addr);
            memcpy(out,&v,size);
            break;
        }

        case 2:
        {
            const u16 v = read_u16<false>(n64,addr);
            memcpy(out,&v,size);
            break;            
        }

        case 4:
        {
            const u32 v = read_u32<false>(n64,addr);
            memcpy(out,&v,size);
            break;                    
        }

        default: return false;
    }
    

    return true;
}

const char* reg_name(u32 idx) 
{
    return beyond_all_repair::REG_NAMES[idx];
}


void reset(N64 &n64, const std::string &filename)
{
    reset_mem(n64.mem,filename);
    reset_cpu(n64);
    reset_rdp(n64);
    n64.size_change = false;

    // initializer external disassembler
    n64.program = beyond_all_repair::make_program(0xA4000040,false,&read_func,&n64);
}

template<const b32 debug>
void run_internal(N64 &n64)
{
    n64.rdp.frame_done = false;

    // dont know how our vblank setup works
    while(!n64.rdp.frame_done)
    {

        while(!n64.scheduler.event_ready())
        {

#ifdef DEBUG
            if constexpr(debug)
            {
                if(n64.debug.is_halted())
                {
                    return;
                }
            }
#endif
            step<debug>(n64);
        }
        n64.scheduler.service_events();
    }

    // dont know when the rendering should be finished just do at end for now
    render(n64);
}


void run(N64& n64)
{
    if(n64.debug_enabled)
    {
        run_internal<true>(n64);
    }

    else
    {
        run_internal<false>(n64);
    }
}
}