#include <gb/gb.h>
 

namespace gameboy
{

//CHANNEL 3 WAVE

//09 of blarggs test will be off due to the apu operating
// 2 cycles at a time

Wave::Wave(GB &gb, int c) :  Channel(gb,c), FreqReg(gb,c), cpu(gb.cpu), apu(gb.apu)
{

}

void Wave::init() noexcept
{
	init_channel();
	freq_init();
}


void Wave::wave_trigger() noexcept
{
/*
	// i think think this is only relevant while its being read...

	// if triggered while on the first four bytes will
	// get set to what the current 4 byte section...
	// unless the channel is reading the first 4 in which case
	// only the first will be overwritten with the current
	if(!cpu.get_cgb() && apu.chan_enabled(2))
	{
		const uint32_t byte = duty_idx / 2;

		if(byte < 4)
		{
			mem.io[0x30] = mem.io[0x30 + byte];
		}

		else
		{
			memcpy(&mem.io[0x30],&mem.io[0x30+(byte &~3)],4);
		}
	}
*/
	reset_duty();
}

void Wave::tick_period(uint32_t cycles) noexcept
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