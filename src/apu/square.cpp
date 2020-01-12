#include "../headers/apu.h"
#include "../headers/memory.h"

// CHANNEL 1 & 2 SQAURE WAVE


void Square::init(Memory *m, int c)
{
    init_channel(m,c);
	freq_init();
	env_init();
    freq_lower_mask = freq_lower_masks[chan_number];
    period_scale = freq_period_scales[chan_number];
	cur_duty = 0;
}

void Square::write_cur_duty(uint8_t v)
{
    cur_duty = (v >> 6) & 0x3;    
}

void Square::tick_period(int cycles)
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

void Square::duty_trigger()
{
	duty_idx = 0;
}