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
            return ((key_cnt >> 8) & 3) | irq_enable_flag << 14 | irq_cond << 15;
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
            key_cnt |= (v << 8);
            irq_enable_flag = is_set(v,6);
            irq_cond = is_set(v,7);
            break;
        }
    }
}



MemIo::MemIo()
{
    init();
}

void MemIo::init()
{
    keyinput = 0x3ff;
    key_control.init();
}

}