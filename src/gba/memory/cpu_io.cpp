#include <gba/cpu_io.h>

namespace gameboyadvance
{

CpuIo::CpuIo()
{
    init();
}


void CpuIo::init()
{
    ime = true;
    interrupt_enable = 0;
    interrupt_flag = 0;    
}

}