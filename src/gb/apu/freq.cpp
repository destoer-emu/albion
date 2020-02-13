#include <gb/apu.h>
 
namespace gameboy
{

// CHANNEL 1,2,3 FREQUENCY


void FreqReg::freq_init()
{
	freq = 0;
	period = 0;
    duty_idx = 0;    
}

void FreqReg::freq_write_lower(uint8_t v)
{
    freq = (freq & freq_lower_mask) | v;
}

void FreqReg::freq_write_higher(uint8_t v)
{
    freq = (freq & 0xff) | ((v & 0x7) << 8);    
}

void FreqReg::freq_reload_period()
{
    period = (2048 - freq)*period_scale;
}

int FreqReg::get_duty_idx() const
{
    return duty_idx;
}

}