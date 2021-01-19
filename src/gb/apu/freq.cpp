#include <gb/gb.h>
 
namespace gameboy
{

// CHANNEL 1,2,3 FREQUENCY

FreqReg::FreqReg(int c) : freq_lower_mask(freq_lower_masks[c]), period_scale(freq_period_scales[c])
{

}

int FreqReg::get_period() const noexcept
{
    return period;
}



void FreqReg::freq_init() noexcept
{
	freq = 0;
	period = 0;
    duty_idx = 0;    
}

void FreqReg::freq_write_lower(uint8_t v) noexcept
{
    freq = (freq & freq_lower_mask) | v;
}

void FreqReg::freq_write_higher(uint8_t v) noexcept
{
    freq = (freq & 0xff) | ((v & 0x7) << 8);    
}

void FreqReg::freq_reload_period() noexcept
{
    period = (2048 - freq)*period_scale;
}


int FreqReg::get_duty_idx() const noexcept
{
    return duty_idx;
}

void FreqReg::reset_duty() noexcept
{
	duty_idx = 0;
}

// extra 6 cycle patch can make 09 pass on blarggs
// https://forums.nesdev.com/viewtopic.php?f=20&t=13730
void FreqReg::freq_trigger() noexcept
{
    // reload frequency peroid on trigger
    freq_reload_period();
}

}