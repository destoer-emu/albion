#include <gb/gb.h>

namespace gameboy
{ 

// CHANNEL 1 & 2 SQAURE WAVE

Square::Square(int c,Psg &p) : Channel(c,p), FreqReg(c)
{

}

void Square::init() noexcept
{
	freq_init();
	env_init();
	cur_duty = 0;
}

void Square::write_cur_duty(uint8_t v) noexcept
{
    cur_duty = (v >> 6) & 0x3;    
}

bool Square::tick_period(uint32_t cycles) noexcept
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
		return true;
	}
	return false;
}

void Square::duty_trigger() noexcept
{
	duty_idx = 0;
}

}