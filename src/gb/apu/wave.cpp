#include <gb/gb.h>
 

namespace gameboy_psg
{

//CHANNEL 3 WAVE

//09 of blarggs test will be off due to the apu operating
// 2 cycles at a time

Wave::Wave(int c,Psg &p) :  Channel(c,p), FreqReg(c)
{

}

void Wave::init(psg_mode mode) noexcept
{
	this->mode = mode;
	init_channel();
	freq_init(mode);
	bank_idx = false;
	dimension = false;

	if(mode == psg_mode::cgb)
	{
		static constexpr uint8_t wave_ram_initial[] =
		{
			0x00, 0xFF, 0x00, 0xFF,
			0x00, 0xFF, 0x00, 0xFF, 
			0x00, 0xFF, 0x00, 0xFF, 
			0x00, 0xFF, 0x00, 0xFF
		}; 

		for(int i = 0; i < 16; i++)
		{
			wave_table[0][i] = wave_ram_initial[i];
		}
	}
}


void Wave::wave_trigger() noexcept
{
	reset_duty();
}

bool Wave::tick_period(uint32_t cycles) noexcept
{
	// handle wave ticking (square 3)	
	period -= cycles;
		
	// reload timer and goto the next sample in the wave table
	if(period <= 0)
	{
		// duty is the wave table index for wave channel 
		
		// check by here for the 2nd bank
		if(dimension)
		{
			// about to overflow switch to over bank
			if(duty_idx == 0x1f)
			{
				bank_idx = !bank_idx;
			}
		}

		duty_idx  = (duty_idx + 1) & 0x1f; 

		// dac is enabled
		if(dac_on() && enabled())
		{
			int pos = duty_idx / 2;

			uint8_t byte;
			if(mode != psg_mode::gba)
			{
				byte = wave_table[0][pos];
			}

			else
			{
				byte = wave_table[bank_idx][pos];
			}
				
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
		return true;			
	}
	return false;
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