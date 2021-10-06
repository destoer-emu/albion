#include <gb/apu.h>
#include <gb/gb.h>
 
namespace gameboy_psg
{

// NOTE: noise has no frequency so its not used
static constexpr s32 freq_period_scales[4] = {4,4,2,0};
static constexpr s32 freq_lower_masks[4] = {~0xff,0x700,~0xff,0};
static constexpr s32 max_lengths[] = {0x40,0x40,0x100,0x40};
static constexpr s32 len_masks[] = {0x3f,0x3f,0xff,0x3f};


void init_channels(psg_mode mode, Channel *channels)
{
    for(int i = 0; i < 4; i++)
    {
        channels[i] = {};
        channels[i].period_factor = mode == psg_mode::gba? 4 : 1;
        channels[i].period_scale = freq_period_scales[i];
        channels[i].freq_lower_mask = freq_lower_masks[i];
        channels[i].max_len = max_lengths[i];
        channels[i].len_mask = len_masks[i];
    }
}

void enable_chan(Channel &c)
{
    c.enabled = true;
}

void disable_chan(Channel &c)
{
    c.enabled = false;
}

void tick_lengthc(Channel &c)
{
    if(c.length_enabled)
    {
        // tick the length counter if zero deset it
        // only decrement if not zero
        if(c.lengthc != 0 && --c.lengthc == 0)
        {
            disable_chan(c);
        }
    }
}

void tick_length_counters_internal(Channel *channels)
{
    for(int i = 0; i < 4; i++)
    {
        tick_lengthc(channels[i]);
    }
}

void length_trigger(Channel &c)
{
    enable_chan(c);

    if(!c.lengthc)
    {
        c.lengthc = c.max_len;


        // disable the chan length
        // if the value enables the length this will cause an extra tick :P
        // disable chan in NRX4
        c.length_enabled = false; 
    }
}

void check_dac(Channel &c)
{
    if(!c.dac_on)
    {
        disable_chan(c);
    }
}

void write_lengthc(Channel &c, u8 v)
{
    c.lengthc = c.max_len - (v & c.len_mask);
}

void length_write(Channel &c, u8 v, u8 seq_step)
{
    // if previously clear and now is enabled 
    // + next step doesent clock, clock the length counter
    if(!c.length_enabled && is_set(v,6)  && (seq_step & 1))
    {
        // if non zero decremt
        if(c.lengthc)
        {
            if(!--c.lengthc)
            {
                // if now zero it goes off if trigger is clear
                if(!is_set(v,7))
                {
                    disable_chan(c);
                }

                // else it becomes max len - 1
                else
                {
                    c.lengthc = c.max_len - 1;
                }
            }
            
        }
    }


    c.length_enabled = is_set(v,6);    
}

}