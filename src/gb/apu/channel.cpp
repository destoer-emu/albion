#include <gb/apu.h>
#include <gb/gb.h>
 
namespace gameboy
{

// SHARED CHANNEL FUNCTIONS

bool Channel::enabled() const noexcept
{
    return is_set(mem.io[IO_NR52],chan_number);
}

void Channel::disable_chan() noexcept
{
    mem.io[IO_NR52] = deset_bit(mem.io[IO_NR52],chan_number);
}

void Channel::enable_chan() noexcept
{
    mem.io[IO_NR52] = set_bit(mem.io[IO_NR52],chan_number);    
}

void Channel::tick_lengthc() noexcept
{
    if(length_enabled)
    {
        // tick the length counter if zero deset it
        if(!--lengthc)
        {
            disable_chan();
        }
    }
}

void Channel::reset_length() noexcept
{
    lengthc = 0;
}

Channel::Channel(GB &gb, int c) : mem(gb.mem),apu(gb.apu), chan_number(c), trigger_addr(trigger_regs[c]),
    max_len(max_lengths[c]),len_mask(len_masks[c]), dac_reg(dac_regs[c]), dac_mask(dac_masks[c])
{

} 

void Channel::init_channel() noexcept
{
    lengthc = 0;
    length_enabled = false;
    output = 0;
}

int Channel::get_output() const noexcept 
{
    return output;
}

// done upon NRX4 write
void Channel::length_trigger() noexcept
{
    enable_chan();

    // if the length counter is 0 it should be loaded with max upon a trigger event
    if(!lengthc)
    {
        lengthc = max_len;
        
        // disable the chan length
        // if the value enables the length this will cause an extra tick :P
        // disable chan in NRX4
        mem.io[trigger_addr] = deset_bit(mem.io[trigger_addr],6); 
    }
}


void Channel::length_write(uint8_t v) noexcept
{

    auto sequencer_step = apu.get_sequencer_step();

    // if previously clear and now is enabled 
    // + next step doesent clock, clock the length counter
    if(!is_set(mem.io[trigger_addr],6) && is_set(v,6) && !(sequencer_step & 1))
    {
        // if not zero decrement
        if(lengthc != 0)
        {	
            // decrement and if now zero and there is no trigger 
            // switch the channel off
            if(!--lengthc)
            {
                if(is_set(v,7)) 
                { 
                    // if we have hit a trigger it should be max len - 1
                    lengthc = max_len - 1;
                }
                    
                else
                {
                    disable_chan();
                }       
            }	
        }	
    }

	// after all the trigger events have happend
	// if the dac is off switch channel off				
	check_dac();

    // bit 6 enables legnth counter
    length_enabled = is_set(v,6);  
}

// length counter write = max len - bits in reg
void Channel::write_lengthc(uint8_t v) noexcept
{
    lengthc = max_len - (v & len_mask);
}

bool Channel::dac_on() const noexcept
{
    return (mem.io[dac_reg] & dac_mask);    
}

void Channel::check_dac() noexcept
{
    if(!dac_on())
    {
        disable_chan();
    }
}

}