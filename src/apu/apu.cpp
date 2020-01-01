#include "../headers/apu.h"
#include "../headers/memory.h"


void Channel::disable_chan()
{
    mem->io[IO_NR52] = deset_bit(mem->io[IO_NR52],chan_number);
}

void Channel::enable_chan()
{
    mem->io[IO_NR52] = set_bit(mem->io[IO_NR52],chan_number);    
}

void Channel::tick_lengthc()
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

void Channel::init_channel(Memory *mem, int chan_number)
{
    this->mem = mem;
    this->chan_number = chan_number;
    trigger_addr = trigger_regs[chan_number];
    max_len = max_lengths[chan_number];
    len_mask = len_masks[chan_number];
    dac_reg = dac_regs[chan_number];
    dac_mask = dac_masks[chan_number];

    lengthc = 0;
    length_enabled = false;
	period = 0;
	volume = 0;
}

// done upon NRX4 write
void Channel::length_trigger(uint8_t v, int sequencer_step)
{
    enable_chan();

    // if the length counter is 0 it should be loaded with max upon a trigger event
    if(lengthc == 0)
    {
        lengthc = max_len;
        
        // disable the chan length
        // if the value enables the length this will cause an extra tick :P
        // disable chan in NRX4
        mem->io[trigger_addr] = deset_bit(mem->io[trigger_addr],6); 
    }

    // if previously clear and now is enabled 
    // + next step doesent clock, clock the length counter
    if(!is_set(mem->io[trigger_addr],6) && is_set(v,6) && !(sequencer_step & 1))
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
                    lengthc = max_len-1;
                }
                    
                else
                {
                    disable_chan();
                }
                    
            }	
        }	
    }

    check_dac();

    // bit 6 enables legnth counter
    length_enabled = is_set(v,6);        
}

void Channel::enable_lengthc()
{
    length_enabled = true;
}

void Channel::set_lengthc(int c)
{
    lengthc = c;
}

void Channel::write_lengthc(uint8_t v)
{
    lengthc = max_len - (v & len_mask);
}

void Channel::check_dac()
{
    if((mem->io[dac_reg] & dac_mask) == 0)
    {
        disable_chan();
    }
}


void Apu::init(Memory *mem)
{
    this->mem = mem;

    // init every channel
    c1.init_channel(mem,0);
    c2.init_channel(mem,1);
    c3.init_channel(mem,2);
    c4.init_channel(mem,3);

	sequencer_step = 0;
	sound_enabled = true;    
}

void Apu::advance_sequencer()
{
	// go to the next step
	sequencer_step += 1;
		
	if(sequencer_step == 8)
	{
		sequencer_step = 0; // loop back around the steps
	}
		
	// switch and perform the function required for our step
	switch(sequencer_step)
	{
		case 0: // clock the length counters
		{
			tick_length_counters();
			break;
		}
			
		case 1: break; // do nothing

			
		case 2: // sweep generator + lengthc
		{
			tick_length_counters();
			//clock_sweep();
			break;
		}
			
		case 3: break; // do nothing

			
		case 4: // clock lengthc
		{
			tick_length_counters();
			break;
		}
			
		case 5: break; // do nothing
			
		case 6:  // clock the sweep generator + lengthc
		{
			tick_length_counters();
			//clock_sweep();
			break;
		}
			
		case 7:
		{ 
			//clock_envelope();
			break; //clock the envelope 
		}
		default:
		{
            throw std::runtime_error("unknown sequencer step");
		}
	}	
}


void Apu::tick_length_counters()
{
    c1.tick_lengthc();
    c2.tick_lengthc();
    c3.tick_lengthc();
    c4.tick_lengthc();
}

void Apu::disable_sound()
{
    // set nr10-nr51 regs to 0
    for(int i = 0x10; i < 0x26; i++)
    {
        mem->write_io(i,0);	
    } 
    
    mem->io[IO_NR52] = 112; // need to write the unused bits and just zero everything else

    // now lock writes
    sound_enabled = false;  
}

void Apu::enable_sound()
{
    sound_enabled = true;
    mem->io[IO_NR52] |= 0x80; // data had 0x80 so write back  
}

void Apu::reset_sequencer()
{
    sequencer_step = 0;
}

int Apu::get_sequencer_step() const
{
    return sequencer_step;
}

bool Apu::chan_enabled(int chan)
{
    return is_set(mem->io[IO_NR52],chan);
}

bool Apu::enabled() const
{
    return sound_enabled;
}