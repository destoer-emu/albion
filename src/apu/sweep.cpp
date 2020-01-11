#include "../headers/apu.h"
#include "../headers/memory.h"

// CHANNEL 1 SWEEP

void Sweep::sweep_init()
{
	sweep_enabled = false;
	sweep_shadow = 0;
	sweep_period = 0;
	sweep_timer = 0;
	sweep_calced = false;
    sweep_reg = 0;  
}


void Sweep::sweep_write(uint8_t v)
{
    
    // if we have used a sweep calc in negation mode 
    // since the last trigger turn the channel off
    if(is_set(sweep_reg,3) && !is_set(v,3))
    {
        if(sweep_calced)
        {
            disable_chan();
        }
    }
    
    sweep_period = (v >> 4) & 7;
    sweep_reg = v;
}

void Sweep::sweep_trigger()
{
    /*
    Square 1's frequency is copied to the shadow register.
    The sweep timer is reloaded.
    The internal enabled flag is set if either the sweep period or shift are non-zero, cleared otherwise.
    If the sweep shift is non-zero, frequency calculation and the overflow check are performed immediately.
    */
    
    sweep_calced = false; // indicates no sweep freq calcs have happened since trigger	
        
    // Copy the freq to the shadow reg
    sweep_shadow = freq;	
        
    // reload the sweep timer
    sweep_period = (sweep_reg >> 4) & 7;

	// sweep period of 0 treated as 8
	sweep_timer = sweep_period?  sweep_period : 8;				
        
    // if sweep period or shift are non zero set the internal flag 
    // else clear it
    // 0x77 is both shift and peroid
    sweep_enabled = (sweep_reg & 0x77);

    // if the sweep shift is non zero 
    // perform the overflow check and freq calc immediately 
    if(sweep_reg & 0x7)
    {
        calc_freqsweep();
    }
}

uint16_t Sweep::calc_freqsweep()
{
	uint16_t shadow_shifted = sweep_shadow >> (sweep_reg & 0x7);
	uint16_t result;
	if(is_set(sweep_reg,3)) // test bit 3 of nr10 if its 1 take them away
	{
		sweep_calced = true; // calc done using negation
		result = sweep_shadow - shadow_shifted;
	}
	
	// else add them
	else
	{
		result = sweep_shadow + shadow_shifted;
	}

	// result is greater than 0x7ff (disable the channel)
	if(result > 0x7ff)
	{
		sweep_timer = 0;
		disable_chan();
		sweep_enabled = false;	
	}

	return result;
}

// calc the freq sweep and do the overflow checking
// disable chan if it overflows
void Sweep::do_freqsweep()
{
	// dont calc the freqsweep if sweep peroid is zero or its disabled
	if(!sweep_enabled || !sweep_period ) { return; }
	
	uint16_t temp = calc_freqsweep();
	
	// sweep shift not zero 
	if( (sweep_reg & 0x7) && temp <= 0x7ff)
	{
		// write back to shadow
		sweep_shadow = temp;
		
		// write back low 8
		mem->io[IO_NR13] = sweep_shadow & 0xff;

		
		// and high 3
		mem->io[IO_NR14] &= ~0x7; // mask bottom 3
		mem->io[IO_NR14] |= (sweep_shadow >> 8) & 0x7; // and write them out
		
		// also back to our internal cached freq
		freq = sweep_shadow;		

		// reperform the calc + overflow check (but dont write back)
		calc_freqsweep();		
	}
}


// clock sweep generator for square 1
void Sweep::clock_sweep()
{
	// tick the sweep timer calc when zero
	if(sweep_timer && !--sweep_timer)
	{
		// if elapsed do the actual "sweep"
		do_freqsweep();
		
		// and reload the timer of course
		// does this use the value present during the trigger event 
		// or is it reloaded newly?

		// sweep period of 0 treated as 8
		sweep_timer = sweep_period?  sweep_period : 8;		
	}	
}