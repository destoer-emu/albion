#include <gb/apu.h>
 
namespace gameboy
{

// CHANNEL 1,2,3 FREQUENCY


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

void FreqReg::freq_trigger() noexcept
{
    // reload frequency peroid on trigger
    freq_reload_period();
}

}