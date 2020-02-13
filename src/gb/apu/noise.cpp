#include <gb/apu.h>
#include <gb/memory.h>

namespace gameboy
{

// CHANNEL 4 NOISE

void Noise::init(Memory *m, int c)
{
    init_channel(m,c);
    env_init();
	clock_shift = 0;
	counter_width = 0;
	divisor_idx = 0; // indexes into divisors table
	shift_reg = 0; // 15 bit reg    
	period = 0;
}

void Noise::tick_period(int cycles)
{
	period -= cycles; // polynomial counter

	if(period <= 0)
	{
		// "The noise channel's frequency timer period is set by a base divisor shifted left some number of bits. "
		period = (divisors[divisor_idx] << clock_shift); 

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
	}
}


void Noise::noise_write(uint8_t v)
{
    divisor_idx = v & 0x7;
    counter_width = is_set(v,3);
    clock_shift = (v >> 4) & 0xf;    
}

void Noise::noise_trigger()
{
    // noise channel stuff
    period = (divisors[divisor_idx] << clock_shift);
    shift_reg = 0x7fff;
}

}