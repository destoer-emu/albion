#include <gb/gb.h>

namespace gameboy
{

Noise::Noise(int c,Psg &p) : Channel(c,p)
{

}

// CHANNEL 4 NOISE

void Noise::init() noexcept
{
    init_channel();
    env_init();
	clock_shift = 0;
	counter_width = 0;
	divisor_idx = 0; // indexes into divisors table
	shift_reg = 0; // 15 bit reg    
	period = 0;
}

bool Noise::tick_period(uint32_t cycles) noexcept
{
	period -= cycles; // polynomial counter

	if(period <= 0)
	{
		reload_period();

		// bottom two bits xored and reg shifted right
		int result = shift_reg & 0x1;
		shift_reg >>= 1;
		result ^= shift_reg & 0x1;

		// result placed in high bit (15 bit reg)
		shift_reg |=  (result << 14);

		if(counter_width) // in width mode
		{
			// also put result in bit 6
			shift_reg = deset_bit(shift_reg,6);
			shift_reg |= result << 6;
		} 

		// if lsb NOT SET
		// put output
		if(enabled() && dac_on() && !is_set(shift_reg,0))
		{
			output = volume;
		}
	
		else 
		{
			output = 0;
		}
		return true;
	}
	return false;
}


void Noise::noise_write(uint8_t v) noexcept
{
    divisor_idx = v & 0x7;
    counter_width = is_set(v,3);
    clock_shift = (v >> 4) & 0xf;    
}

void Noise::noise_trigger() noexcept
{
    shift_reg = 0x7fff;
}

void Noise::reload_period() noexcept
{
	// "The noise channel's frequency timer period is set by a base divisor shifted left some number of bits. "
	period = divisors[divisor_idx] << clock_shift;
}


int Noise::get_period() const noexcept
{
	return period;
}

}