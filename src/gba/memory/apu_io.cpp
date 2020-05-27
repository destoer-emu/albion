#include <gba/apu_io.h>

namespace gameboyadvance
{



SoundCnt::SoundCnt(ApuIo &io) : apu_io(io)
{
    init();
}

void SoundCnt::init()
{
    // sound cnt l
    vol_right = 0;
    vol_left = 0;
    right_enable = 0;
    left_enable = 0;

    // soundcnt h
    psg_vol = 0;
    dma_vol_a = false;
    dma_vol_b = false;

    enable_right_a = false;
    enable_left_a = false;
    timer_num_a = false;


    enable_right_b = false;
    enable_left_b = false;
    timer_num_b = false ;


    // soundcnt x
    sound1_enable = false;
    sound2_enable = false;
    sound3_enable = false;
    sound4_enable = false;
    sound_enable = false;    
}



void SoundCnt::write_h(int idx, uint8_t v)
{
    switch(idx)
    {
        case 0:
        {
            psg_vol = v & 0x3;
            dma_vol_a = is_set(v,2);
            dma_vol_b = is_set(v,3);
            break;
        }

        case 1:
        {
            enable_right_a = is_set(v,0);
            enable_left_a = is_set(v,1);
            timer_num_a = is_set(v,2);

            // fifo reset handed upon a 1 written here...
            if(is_set(v,3))
            {
                apu_io.fifo_a.init();
            }

            enable_right_b = is_set(v,4);
            enable_left_b = is_set(v,5);
            timer_num_b = is_set(v,6);

            // and the same for b by here...
            if(is_set(v,7))
            {
                apu_io.fifo_b.init();
            }
            break;
        }
    }
}

uint8_t SoundCnt::read_h(int idx) const
{
    switch(idx)
    {
        case 0:
        {
            return psg_vol  | (dma_vol_a << 2) | (dma_vol_b << 3);
            break;
        }

        case 1:
        {
            return enable_right_a | (enable_left_a << 1) | (timer_num_a << 2) |
                (enable_right_b << 4) | (enable_left_b << 5) | (timer_num_b << 6);
            break;
        }
    }

    return 0;
}



SoundFifo::SoundFifo()
{
    init();
}

void SoundFifo::init()
{
    memset(fifo,0,sizeof(fifo));
    len = 0;
    read_idx = 0;
    write_idx = 0;
}

void SoundFifo::write(uint8_t v)
{
    if(len < 32)
    {
        fifo[write_idx] = v;
        write_idx = (write_idx + 1) & 31;
        len++;
    }

}

uint8_t SoundFifo::read()
{

    uint8_t res = 0;

    if(len > 0)
    {
        res = fifo[read_idx];
        read_idx = (read_idx + 1) & 31;
        len--;
    }

    return res;
}

ApuIo::ApuIo()
{
    init();
}

void ApuIo::init()
{
    sound_cnt.init();
    fifo_a.init();
    fifo_b.init();
}

}
