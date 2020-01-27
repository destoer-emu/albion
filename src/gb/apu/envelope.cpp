#include <gb/apu.h>
 

// CHANNEL 1,2,4 envelope

void Envelope::env_init()
{
	env_period = 0; 
	env_load = 0;
	volume = 0;
	volume_load = 0;
	env_enabled = false; 
	env_up = false;    
}

void Envelope::env_write(uint8_t v)
{
    volume_load = (v >> 4) & 0xf;
    volume = volume_load;
    env_load = v & 0x3;
    env_up = is_set(v,3);
}

void Envelope::env_trigger()
{
    env_period = env_load;				
    volume = volume_load;
    env_enabled = true;    
}

void Envelope::clock_envelope()
{
	int vol = volume;
	if(!--env_period && env_enabled)
    {
		// sweep up or sweep down?
		vol += env_up ? +1 : -1;

		if(vol >= 0 && vol <= 15) // if vol is between 0 and 15 it is updated
		{
			volume = vol;
		}

		else
		{
			env_enabled = false;
		}

		// period of zero treated as 8
		env_period = env_load? env_load : 8;
    }    
}