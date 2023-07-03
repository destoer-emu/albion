#include <gb/apu.h>

namespace gameboy_psg
{

void init_noise(Noise &noise)
{
	noise = {};
}

void noise_write(Noise &n,u8 v)
{
    n.divisor_idx = v & 0x7;
    n.counter_width = is_set(v,3);
    n.clock_shift = (v >> 4) & 0xf;    
}

void noise_trigger(Noise &n)
{
    n.shift_reg = 0x7fff;
}

//http://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware#Noise_Channel
static constexpr u32 divisors[8] = { 8, 16, 32, 48, 64, 80, 96, 112 };

void noise_reload_period(Channel &c,Noise &n)
{
	// "The noise channel's frequency timer period is set by a base divisor shifted left some number of bits. "
	c.period = (divisors[n.divisor_idx] << n.clock_shift) * c.period_factor;	
}

bool noise_tick_period(Noise &n,Channel &c, u32 cycles)
{
	c.period -= cycles; // polynomial counter

	if(c.period <= 0)
	{
		noise_reload_period(c,n);

		// bottom two bits xored and reg shifted right
		int result = n.shift_reg & 0x1;
		n.shift_reg >>= 1;
		result ^= n.shift_reg & 0x1;

		// result placed in high bit (15 bit reg)
		n.shift_reg |=  (result << 14);

		if(n.counter_width) // in width mode
		{
			// also put result in bit 6
			n.shift_reg = deset_bit(n.shift_reg,6);
			n.shift_reg |= result << 6;
		} 

		// if lsb NOT SET
		// put output
		if(c.enabled && c.dac_on && !is_set(n.shift_reg,0))
		{
			c.output = c.volume;
		}
	
		else 
		{
			c.output = 0;
		}
		return true;
	}
	return false;
}

}