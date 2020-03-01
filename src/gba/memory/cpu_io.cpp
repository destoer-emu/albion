#include <gba/cpu_io.h>


namespace gameboyadvance
{


HaltCnt::HaltCnt()
{
    init();
}

void HaltCnt::init()
{
    state = power_state::normal;
}

void HaltCnt::write(uint8_t v)
{
    state = is_set(v,7) ? power_state::stop : power_state::halt;
}


CpuIo::CpuIo()
{
    init();
}


void CpuIo::init()
{
    ime = true;
    interrupt_enable = 0;
    interrupt_flag = 0; 
    halt_cnt.init(); 
}

}