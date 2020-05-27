#include <gb/gb.h>
 

namespace gameboy
{

//CHANNEL 3 WAVE

Wave::Wave(GB &gb, int c) : Channel(gb,c), FreqReg(gb,c)
{

}

void Wave::init() noexcept
{
	init_channel();
	freq_init();
}


void Wave::wave_trigger() noexcept
{
    // reset the duty
    duty_idx = 0;
}

void Wave::tick_period(int cycles) noexcept
{
	// handle wave ticking (square 3)	
	period -= cycles;
		
	// reload timer and goto the next sample in the wave table
	if(period <= 0)
	{
		// duty is the wave table index for wave channel 
		duty_idx  = (duty_idx + 1) & 0x1f; 

		// dac is enabled
		if(dac_on() && enabled())
		{
			int pos = duty_idx / 2;
			uint8_t byte = mem.io[0x30 + pos];
				
			if(!is_set(duty_idx,0)) // access the high nibble first
			{
				byte >>= 4;
			}
				
			byte &= 0xf;
				
			if(volume)
			{
				byte >>= volume - 1;
			}
				
			else
			{
				byte = 0;
			}
			output = byte;
			
		}
			
		else
		{ 
			output = 0;
		}

		// reload the timer
		// period (2048-frequency)*2 (in cpu cycles)
		freq_reload_period();			
	}
}


void Wave::vol_trigger() noexcept
{
    volume = volume_load;
}

void Wave::write_vol(uint8_t v) noexcept
{
    volume_load = (v >> 5) & 0x3;
    volume = volume_load;    
}

}