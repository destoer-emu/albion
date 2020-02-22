#include <gba/cpu_io.h>

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