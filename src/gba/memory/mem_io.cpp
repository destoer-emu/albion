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

uint8_t KeyCnt::read(int idx) const
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

void KeyCnt::write(int idx, uint8_t v)
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


void SioCnt::write(int idx, uint8_t v)
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


uint8_t SioCnt::read(int idx) const
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
}

}