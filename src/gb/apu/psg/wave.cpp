#include <gb/apu.h>
 

namespace gameboy_psg
{

void init_wave(Wave &w,psg_mode mode)
{
	w = {};

    w.mode = mode;

    if(mode == psg_mode::cgb)
    {
        static constexpr u8 WAVE_RAM_INITIAL[] = 
		{
			0x00, 0xFF, 0x00, 0xFF,
			0x00, 0xFF, 0x00, 0xFF, 
			0x00, 0xFF, 0x00, 0xFF, 
			0x00, 0xFF, 0x00, 0xFF
		}; 

		for(int i = 0; i < 16; i++)
		{
			w.table[0][i] = WAVE_RAM_INITIAL[i];
		}
    }
}

void wave_trigger(Channel &c)
{
    // reset duty
    c.duty_idx = 0;
}

void wave_vol_trigger(Channel &c)
{
    c.volume = c.volume_load;
}

void wave_write_vol(Channel &c, u8 v)
{
    c.volume_load = (v >> 5) & 0x3;
    c.volume = c.volume_load;
}

bool wave_tick_period(Wave &w, Channel &c, u32 cycles)
{
	// handle wave ticking (square 3)	
	c.period -= cycles;
		
	// reload timer and goto the next sample in the wave table
	if(c.period <= 0)
	{
		// duty is the wave table index for wave channel 
		
		// check by here for the 2nd bank
		if(w.dimension)
		{
			// about to overflow switch to over bank
			if(c.duty_idx == 0x1f)
			{
				w.bank_idx = !w.bank_idx;
			}
		}

		c.duty_idx  = (c.duty_idx + 1) & 0x1f; 

		// dac is enabled
		if(c.dac_on && c.enabled)
		{
			int pos = c.duty_idx / 2;

			u8 byte;
			if(w.mode != psg_mode::gba)
			{
				byte = w.table[0][pos];
			}

			else
			{
				byte = w.table[w.bank_idx][pos];
			}
				
			if(!is_set(c.duty_idx,0)) // access the high nibble first
			{
				byte >>= 4;
			}
				
			byte &= 0xf;
				
			if(c.volume)
			{
				byte >>= c.volume - 1;
			}
				
			else
			{
				byte = 0;
			}

			c.output = byte;
		}
			
		else
		{ 
			c.output = 0;
		}

		// reload the timer
		// period (2048-frequency)*2 (in cpu cycles)
		freq_reload_period(c);
		return true;			
	}
	return false;
}


}