#include <psg/psg.h>


namespace gameboy_psg
{

void env_write(Channel &c, u8 v)
{
    c.volume_load = (v >> 4) & 0xf;
    c.volume = c.volume_load;
    c.env_load = v & 0x7;
    c.env_up = is_set(v,3);    
}

void env_trigger(Channel &c)
{
    c.env_period = c.env_load;				
    c.volume = c.volume_load;
    c.env_enabled = true;   
}

void clock_envelope(Channel &c)
{
	int vol = c.volume;
	if(!--c.env_period && c.env_enabled)
    {
		// sweep up or sweep down?
		vol += c.env_up ? +1 : -1;

		if(vol >= 0 && vol <= 15) // if vol is between 0 and 15 it is updated
		{
			c.volume = vol;
		}

		else
		{
			c.env_enabled = false;
		}

		// period of zero treated as 8
		c.env_period = c.env_load? c.env_load : 8;
    }       
}

}