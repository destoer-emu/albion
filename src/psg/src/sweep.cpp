#include <psg/psg.h>

namespace gameboy_psg
{ 

void init_sweep(Sweep &sweep)
{
	sweep = {};
}

u16 calc_freqsweep(Channel &c, Sweep &s)
{
	const u16 shadow_shifted = s.shadow >> (s.reg & 0x7);
	u16 result;
	if(is_set(s.reg,3)) // test bit 3 of nr10 if its 1 take them away
	{
		s.calced = true; // calc done using negation
		result = s.shadow - shadow_shifted;
	}
	
	// else add them
	else
	{
		result = s.shadow + shadow_shifted;
	}

	// result is greater than 0x7ff (disable the channel)
	if(result > 0x7ff)
	{
		s.timer = 0;
		disable_chan(c);
		s.enabled = false;	
	}

	return result;
}

void do_freqsweep(Psg &psg)
{
	auto &s = psg.sweep;
	auto &c = psg.channels[0];

	// dont calc the freqsweep if sweep peroid is zero or its disabled
	if(!s.enabled || !s.period ) { return; }
	
	const u16 temp = calc_freqsweep(c,s);
	
	// sweep shift not zero 
	if( (s.reg & 0x7) && temp <= 0x7ff)
	{
		// write back to shadow
		s.shadow = temp;
		
		psg.nr1_freq_writeback(s.shadow);
		
		// also back to our internal cached freq
		c.freq = s.shadow;		

		// reperform the calc + overflow check (but dont write back)
		calc_freqsweep(c,s);		
	}
}

void clock_sweep(Psg &psg)
{
	auto &s = psg.sweep;

	// tick the sweep timer calc when zero
	if(s.timer && !--s.timer)
	{
		// if elapsed do the actual "sweep"
		do_freqsweep(psg);
		
		// and reload the timer of course
		// does this use the value present during the trigger event 
		// or is it reloaded newly?

		// sweep period of 0 treated as 8
		s.timer = s.period?  s.period : 8;		
	}		
}

void sweep_write(Sweep &s, Channel &c,u8 v) 
{
    // if we have used a sweep calc in negation mode 
    // since the last trigger turn the channel off
    if(is_set(s.reg,3) && !is_set(v,3))
    {
        if(s.calced)
        {
            disable_chan(c);
        }
    }
    
    s.period = (v >> 4) & 7;
    s.reg = v;
}

// need to impl this again as most of the documentation is wrong for edge cases...
// there is not an internal enable flag at all
void sweep_trigger(Sweep &s, Channel &c)
{
    /*
    Square 1's frequency is copied to the shadow register.
    The sweep timer is reloaded.
    The internal enabled flag is set if either the sweep period or shift are non-zero, cleared otherwise.
    If the sweep shift is non-zero, frequency calculation and the overflow check are performed immediately.
    */
    
    s.calced = false; // indicates no sweep freq calcs have happened since trigger	
        
    // Copy the freq to the shadow reg
    s.shadow = c.freq;	
        
    // reload the sweep timer
    s.period = (s.reg >> 4) & 7;

	// sweep period of 0 treated as 8
	s.timer = s.period?  s.period : 8;				
        
    // if sweep period or shift are non zero set the internal flag 
    // else clear it
    // 0x77 is both shift and peroid
    s.enabled = (s.reg & 0x77);

    // if the sweep shift is non zero 
    // perform the overflow check and freq calc immediately 
    if(s.reg & 0x7)
    {
        calc_freqsweep(c,s);
    }	
}

}