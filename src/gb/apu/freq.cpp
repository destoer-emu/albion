#include <gb/gb.h>
 
namespace gameboy
{

// CHANNEL 1,2,3 FREQUENCY

FreqReg::FreqReg(GB &gb,int c) : freq_lower_mask(freq_lower_masks[c]), period_scale(freq_period_scales[c]), 
    scheduler(gb.scheduler), channel_event(channel_events[c])
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

    // create  a new event as the period has changed
    // need to half to double the ammount in double speed
    // so it still operates as if it was at 4mhz
    const auto event = scheduler.create_event(period << scheduler.is_double(),channel_event);

    // dont tick off the old event as 
    // it will use the new value as we have just overwritten 
    // the old internal counter
    // this is not an event we are dropping and expecting to start later 

    scheduler.insert(event,false);

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