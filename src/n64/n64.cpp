#include <n64/n64.h>

namespace nintendo64
{

void reset(N64 &n64, const std::string &filename)
{
    UNUSED(filename);
    UNUSED(n64);
    reset_mem(n64.mem,filename);
    reset_cpu(n64.cpu);
}

void run(N64 &n64)
{
    UNUSED(n64);
    // dont know how our vblank setup works
    for(;;)
    {
        step(n64);
    }
}

}