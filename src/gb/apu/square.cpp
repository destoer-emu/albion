#include <gb/gb.h>

namespace gameboy_psg
{ 

static constexpr int duty[4][8] = 
{
    {0,0,0,0,0,0,0,1},   // 12.5
    {1,0,0,0,0,0,0,1},   // 25
    {1,0,0,0,0,1,1,1},   // 50 
    {0,1,1,1,1,1,1,0}    // 75
};


bool square_tick_period(Channel &c,u32 cycles)
{

	c.period -= cycles;

	if(c.period <= 0)
	{
		// advance the duty
		c.duty_idx = (c.duty_idx + 1) & 0x7;
		freq_reload_period(c);

		// if channel and dac is enabled
		// output is volume else nothing
		c.output = (c.enabled && c.dac_on)? c.volume : 0;


		// if the duty is on a low posistion there is no output
		// (vol is multiplied by duty but its only on or off)
		c.output *= duty[c.cur_duty][c.duty_idx];
		return true;
	}
	return false;
}

void duty_trigger(Channel &c)
{
    c.duty_idx = 0;
}

void write_cur_duty(Channel &c, u8 v)
{
    c.cur_duty = (v >> 6) & 0x3;
}

}