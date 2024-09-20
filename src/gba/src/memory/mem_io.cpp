#include <gba/mem_io.h>

namespace gameboyadvance
{

KeyCnt::KeyCnt()
{
    init();
}

void KeyCnt::init()
{
    key_cnt = 0; // what button do we care about
    irq_enable_flag = false; // enable irq
    irq_cond = false; // 0 at least one is pressed, 1 all pressed
}

u8 KeyCnt::read(int idx) const
{
    switch(idx)
    {
        case 0:
        {
            return key_cnt & 0xff;
        }

        case 1:
        {
            return ((key_cnt >> 8) & 3) | irq_enable_flag << 6 | irq_cond << 7;
        }
    }    
    return 0;
}

void KeyCnt::write(int idx, u8 v)
{
    switch(idx)
    {
        case 0:
        {
            key_cnt &= ~0xff;
            key_cnt |= v;
            break;
        }

        case 1:
        {
            key_cnt &= ~0xff00;
            key_cnt |= ((v & 3) << 8);
            irq_enable_flag = is_set(v,6);
            irq_cond = is_set(v,7);
            break;
        }
    }
}


SioCnt::SioCnt()
{
    init();
}

void SioCnt::init()
{
    shift_clock = 0;
    internal_shift_clock = 0; // 256khz / 2mhz
    si_state = false;
    so_during_activity = false;
    start = false;
    transfer_length = false;
    irq = false;
}


void SioCnt::write(int idx, u8 v)
{
    switch(idx)
    {
        case 0:
        {
            shift_clock = is_set(v,0);
            internal_shift_clock = is_set(v,1);
            // si state is read only
            so_during_activity = is_set(v,2);
            start = is_set(v,7);
            break;
        }

        case 1:
        {
            transfer_length = is_set(v,4);
            irq = is_set(v,6);
            break;
        }
    }
}


u8 SioCnt::read(int idx) const
{
    switch(idx)
    {
        case 0:
        {
            return shift_clock | 
                internal_shift_clock  << 1 |
                so_during_activity  << 2 |
                start << 7;
            break;
        }

        case 1:
        {
            return transfer_length << 4 |
                irq << 6;
            break;
        }
    }

    return 0;
}



WaitCnt::WaitCnt()
{
    init();
}

void WaitCnt::init()
{
    sram_cnt = 0;
    wait01 = 0;
    wait02 = 0;
    wait11 = 0;
    wait12 = 0;
    wait21 = 0;
    wait22 = 0;
    term_output = 0;
    prefetch = false;
    gamepak_flag = false;
}


void WaitCnt::write(int idx, u8 v)
{
    switch(idx)
    {
        case 0:
        {
            sram_cnt = v & 3;
            wait01 = (v >> 2) & 3;
            wait02 = is_set(v,4);
            wait11 = (v >> 5) & 3;
            wait12 = is_set(v,7);
            break;
        }

        case 1:
        {
            wait21 = v & 3;
            wait22 = is_set(v,2);
            term_output = (v >> 3) & 3;
            prefetch = is_set(v,6);
            break;
        }
    }
}


u8 WaitCnt::read(int idx)
{
    switch(idx)
    {
        case 0:
        {
            return sram_cnt | wait01 << 2 | wait02 << 4
                | wait11 << 5 | wait12 << 7;
            break;
        }

        case 1:
        {
            // hardcode gba cart...
            return wait21 | wait22 << 2 | term_output << 3 
                | prefetch << 6;
            break;
        }
    }
    // should not be reached
    return 0;
}


MemIo::MemIo()
{
    init();
}

void MemIo::init()
{
    keyinput = 0x3ff; 
    postflg = 0;   
    siocnt.init();
    key_control.init();
    wait_cnt.init();
}

}