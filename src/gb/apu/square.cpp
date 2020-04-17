#include <gb/apu.h>
#include <gb/memory.h>

namespace gameboy
{ 

// CHANNEL 1 & 2 SQAURE WAVE

Square::Square(GB &gb, int c) : Channel(gb,c)
{

}

void Square::init() noexcept
{
	freq_init();
	env_init();
    freq_lower_mask = freq_lower_masks[chan_number];
    period_scale = freq_period_scales[chan_number];
	cur_duty = 0;
}

void Square::write_cur_duty(uint8_t v) noexcept
{
    cur_duty = (v >> 6) & 0x3;    
}

void Square::tick_period(int cycles) noexcept
{
	period -= cycles;

	if(period <= 0)
	{
		// advance the duty
		duty_idx = (duty_idx + 1) & 0x7;
		freq_reload_period();

		// if channel and dac is enabled
		// output is volume else nothing
		output = (enabled() && dac_on())? volume : 0;


		// if the duty is on a low posistion there is no output
		// (vol is multiplied by duty but its only on or off)
		output *= duty[cur_duty][duty_idx];
	}
}

void Square::duty_trigger() noexcept
{
	duty_idx = 0;
}

}