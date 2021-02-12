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



Timer::Timer(int timer) : timer_interrupt(timer_interrupts[timer])
{
    assert(timer >= 0 && timer < 4);
    init();
}

void Timer::init()
{
    reload = 0;
    counter = 0;
    scale = 0;
    cycle_count = 0;
    count_up = false;
    irq = false;
    enable = false;
}

uint8_t Timer::read_counter(int idx) const
{
    switch(idx)
    {
        case 0:
        {
            return counter & 0xff;
        }

        case 1:
        {
            return (counter >> 8) & 0xff;
        }
    }
    return 0;
}

// actually writes the reload but is at the same addr
void Timer::write_counter(int idx, uint8_t v)
{
    switch(idx)
    {
        case 0:
        {
            reload = (reload & 0xff00) | v;
            break;
        }

        case 1:
        {
            reload = (reload & 0x00ff) | (v << 8);
            break;
        }
    }
}

uint8_t Timer::read_control() const
{
    return scale | (count_up << 2) | (irq << 6) | (enable << 7);
}

void Timer::write_control(uint8_t v)
{
    scale = v & 0x3;
    count_up = is_set(v,2);
    irq = is_set(v,6);
    const bool cur = is_set(v,7);

    // if we have just enabled the timer reload it
    if(!enable && cur)
    {
        counter = reload;
    }
    
    enable = cur;
}


CpuIo::CpuIo()
{
    init();
}

void CpuIo::init()
{
    ime = false;
    interrupt_enable = 0;
    interrupt_flag = 0; 
    halt_cnt.init(); 


    for(auto &x: timers)
    {
        x.init();
    }
}

}