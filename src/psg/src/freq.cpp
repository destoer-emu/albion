#include <psg/psg.h>
 
namespace gameboy_psg
{

void freq_write_lower(Channel &c, u8 v)
{
    c.freq = (c.freq & c.freq_lower_mask) | v;
}

void freq_write_higher(Channel &c, u8 v)
{
    c.freq = (c.freq & 0xff) | ((v & 0x7) << 8);
}

void freq_reload_period(Channel &c)
{
    c.period = (2048 - c.freq)*c.period_scale*c.period_factor;
}

void freq_trigger(Channel &c)
{
    freq_reload_period(c);
}

}