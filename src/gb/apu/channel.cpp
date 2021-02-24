#include <gb/apu.h>
#include <gb/gb.h>
 
namespace gameboy_psg
{

// SHARED CHANNEL FUNCTIONS

bool Channel::enabled() const noexcept
{
    return psg.chan_enabled(chan_number);
}

void Channel::disable_chan() noexcept
{
    psg.disable_chan(chan_number);
}

void Channel::enable_chan() noexcept
{
    psg.enable_chan(chan_number);
}

void Channel::tick_lengthc() noexcept
{
    if(length_enabled)
    {
        // tick the length counter if zero deset it
        // only decrement if not zero
        if(lengthc != 0 && --lengthc == 0)
        {
            disable_chan();
        }
    }
}

void Channel::reset_length() noexcept
{
    lengthc = 0;
}

Channel::Channel(int c, Psg &p) : psg(p), chan_number(c),
    max_len(max_lengths[c]),len_mask(len_masks[c]), dac_reg(psg.get_dac_ref(c)), dac_mask(dac_masks[c])
{

} 

void Channel::init_channel() noexcept
{
    lengthc = 0;
    length_enabled = false;
    output = 0;
    length_extra_tick = false;
}

int Channel::get_output() const noexcept 
{
    return output;
}

void Channel::length_trigger() noexcept
{

    enable_chan();

    if(!lengthc)
    {
        lengthc = max_len;

        // disable the chan length
        // if the value enables the length this will cause an extra tick :P
        // disable chan in NRX4
        length_enabled = false; 
    }             
}

// also handles trigger effects
void Channel::length_write(uint8_t v) noexcept
{
    const auto sequencer_step = psg.get_sequencer_step();

    // if previously clear and now is enabled 
    // + next step doesent clock, clock the length counter
    if(!length_enabled && is_set(v,6)  && (sequencer_step & 1))
    {
        // if non zero decremt
        if(lengthc)
        {
            if(!--lengthc)
            {
                // if now zero it goes off if trigger is clear
                if(!is_set(v,7))
                {
                    disable_chan();
                }

                // else it becomes max len - 1
                else
                {
                    lengthc = max_len - 1;
                }
            }
            
        }
    }


    length_enabled = is_set(v,6);
}


// length counter write = max len - bits in reg
void Channel::write_lengthc(uint8_t v) noexcept
{
    lengthc = max_len - (v & len_mask);
}

bool Channel::dac_on() const noexcept
{
    return (dac_reg & dac_mask);    
}

void Channel::check_dac() noexcept
{
    if(!dac_on())
    {
        disable_chan();
    }
}

}