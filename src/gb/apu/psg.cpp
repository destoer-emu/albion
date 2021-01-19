#include <gb/gb.h>

namespace gameboy
{

Psg::Psg() : c1{0,*this}, c2{1,*this}, c3{2,*this},c4{3,*this}
{

}

void Psg::init(bool is_cgb)
{
    // init every channel
    c1.init(); c1.sweep_init();
    c2.init();
    c3.init(is_cgb);
    c4.init();

    this->is_cgb = is_cgb;

    sequencer_step = 0;   

    sound_enabled = true;

    nr10 = 0x80;
    nr11 = 0xBF;
    nr12 = 0xF3;
    nr13 = 0x00;
    nr14 = 0xBF;


    nr21 = 0x3f;
    nr22 = 0;
    nr33 = 0;
    nr24 = 0xbf;

    nr30 = 0x7f;
    nr31 = 0xff;
    nr32 = 0x9f;
    nr33 = 0x00;
    nr34 = 0xbf;

    nr41 = 0xff;
    nr22 = 0;
    nr43 = 0;
    nr44 = 0xbf;

	nr50 = 0x77;
    nr51 = 0xF3;
	nr52 = 0xF1;
}

void Psg::advance_sequencer() noexcept
{
	// switch and perform the function required for our step
	switch(sequencer_step)
	{
		case 0: // clock the length counters
		{
			tick_length_counters();
			break;
		}
			
		case 1: break; // do nothing

			
		case 2: // sweep generator + lengthc
		{
			tick_length_counters();
			c1.clock_sweep();
			break;
		}
			
		case 3: break; // do nothing

			
		case 4: // clock lengthc
		{
			tick_length_counters();
			break;
		}
			
		case 5: break; // do nothing
			
		case 6:  // clock the sweep generator + lengthc
		{
			tick_length_counters();
			c1.clock_sweep();
			break;
		}
			
		case 7:
		{ 
			clock_envelopes();
			break; //clock the envelope 
		}
	}

	// go to the next step
	sequencer_step = (sequencer_step + 1) & 7;
}

void Psg::reset_sequencer() noexcept
{
    sequencer_step = 0;
}

int Psg::get_sequencer_step() const noexcept
{
    return sequencer_step;
}

void Psg::tick_length_counters() noexcept
{
    c1.tick_lengthc();
    c2.tick_lengthc();
    c3.tick_lengthc();
    c4.tick_lengthc();
}

void Psg::clock_envelopes() noexcept
{
    c1.clock_envelope();
    c2.clock_envelope();
    c4.clock_envelope();
}

void Psg::tick_periods(uint32_t cycles) noexcept
{
    c1.tick_period(cycles);
    c2.tick_period(cycles);
    c3.tick_period(cycles);
    c4.tick_period(cycles);    
}

void Psg::enable_sound() noexcept
{
    nr52 |= 0x80; // data had 0x80 so write back  
    sound_enabled = true;

    // reset length coutners when powerd up
    // if on cgb
    if(is_cgb)
    {
        c1.reset_length();
        c2.reset_length();
        c3.reset_length();
        c4.reset_length();
    }

    sequencer_step = 0;

    c1.reset_duty();
    c2.reset_duty();
    c3.reset_duty();
}

void Psg::disable_sound() noexcept
{
    // set nr10-nr51 regs to 0
    write_nr10(0);
    write_nr11(0);
    write_nr12(0);
    write_nr13(0);
    write_nr14(0);

    write_nr21(0);
    write_nr22(0);
    write_nr23(0);
    write_nr24(0);
 
    write_nr30(0);
    write_nr31(0);
    write_nr32(0);
    write_nr33(0);
    write_nr34(0);

    write_nr41(0);
    write_nr42(0);
    write_nr43(0);
    write_nr44(0);

    write_nr50(0);
    write_nr51(0);


    nr52 = 112; // need to write the unused bits and just zero everything else

    // now lock writes
    sound_enabled = false;     
}

void Psg::write_nr10(uint8_t v) noexcept
{	
    if(sound_enabled)
    {
        c1.sweep_write(v);
        nr10 = v | 128;
    }
}

uint8_t Psg::read_nr10() const noexcept
{
    return nr10;
}

void Psg::write_nr11(uint8_t v) noexcept
{
    if(sound_enabled || !is_cgb)
    {
        // bottom 6 bits are length data 
        // set the internal counter to 64 - bottom 6 bits of data
        c1.write_lengthc(v);
        // can only be written while on both versions
        if(sound_enabled)
        {
            c1.write_cur_duty(v);
            nr11 = v;
        }
    }    
}

uint8_t Psg::read_nr11() const noexcept
{
    return (nr11 & (128 + 64)) | (0xff-(128+64));
}

void Psg::write_nr12(uint8_t v) noexcept
{
    if(sound_enabled)
    {
        nr12 = v;
        c1.check_dac();
        c1.env_write(v);
    }
}

uint8_t Psg::read_nr12() const noexcept
{
    return nr12;
}

void Psg::write_nr13(uint8_t v) noexcept
{
    if(sound_enabled)
    {
        c1.freq_write_lower(v);
        nr13 = v;
    }    
}

void Psg::write_nr14(uint8_t v) noexcept
{
    if(sound_enabled)
    {
        c1.freq_write_higher(v);
        

        if(is_set(v,7)) // trigger
        {
            c1.length_trigger();
            c1.freq_trigger();
            c1.env_trigger();
            c1.sweep_trigger();
            c1.duty_trigger();
        }


        c1.length_write(v);
        nr14 = v;

        // after all the trigger events have happend
        // if the dac is off switch channel off				
        c1.check_dac();
    }    
}

uint8_t Psg::read_nr14() const noexcept
{
    return (nr14 & (64)) | (0xff-64);  
}

void Psg::write_nr21(uint8_t v) noexcept
{
    if(sound_enabled || !is_cgb)
    {
        c2.write_lengthc(v);
        if(sound_enabled)
        {
            c2.write_cur_duty(v);
            nr21 = v;
        }
    }    
}

uint8_t Psg::read_nr21() const noexcept
{
    return (nr21 & (128 + 64)) | (0xff-(128+64));	
}

void Psg::write_nr22(uint8_t v) noexcept
{
    if(sound_enabled)
    {
        nr22 = v;
        c2.check_dac();	
        c2.env_write(v);
    }    
}

uint8_t Psg::read_nr22() const noexcept
{
    return nr22;
}

void Psg::write_nr23(uint8_t v) noexcept
{
    if(sound_enabled)
    {
        nr23 = v;
        c2.freq_write_lower(v);
    }    
}

void Psg::write_nr24(uint8_t v) noexcept
{
    if(sound_enabled)
    {
        c2.freq_write_higher(v);


        if(is_set(v,7)) // trigger
        {
            c2.length_trigger();
            c2.freq_trigger();
            c2.env_trigger();
            c2.duty_trigger();
        }

        c2.length_write(v);
        nr24 = v;


        c2.check_dac();	
    }    
}

uint8_t Psg::read_nr24() const noexcept
{
    return (nr24 & (64)) | (0xff-64);	  
}

void Psg::write_nr30(uint8_t v) noexcept
{
    if(sound_enabled)
    {
        nr30 = v | 127;
        c3.check_dac();
    }    
}

uint8_t Psg::read_nr30() const noexcept
{
    return (nr30 & (128)) | (0xff-128);
}

void Psg::write_nr31(uint8_t v) noexcept
{
    if(sound_enabled || !is_cgb)
    {
        c3.write_lengthc(v);
        nr31 = v;
    }    
}

void Psg::write_nr32(uint8_t v) noexcept
{
    if(sound_enabled)
    {
        c3.write_vol(v);
        nr32 = v | 159;
    }    
}

uint8_t Psg::read_nr32() const noexcept
{
    return (nr32 & (64 + 32)) | (0xff-(64+32));
}

void Psg::write_nr33(uint8_t v) noexcept
{
    if(sound_enabled)
    {
        c3.freq_write_lower(v);
        nr33 = v;
    }    
}

void Psg::write_nr34(uint8_t v) noexcept
{
    if(sound_enabled)
    {
        c3.freq_write_higher(v);

        if(is_set(v,7)) // trigger
        {
            c3.length_trigger();
            c3.freq_trigger();
            c3.wave_trigger();
            c3.vol_trigger();
        }

        c3.length_write(v);
        nr34 = v | (16 + 32 + 8);

        c3.check_dac();	
    }
}

uint8_t Psg::read_nr34() const noexcept
{
    return (nr34 & (64)) | (0xff-64);
}

void Psg::write_nr41(uint8_t v) noexcept
{
    if(sound_enabled || !is_cgb)
    {
        c4.write_lengthc(v);
        nr41 = v | 192;
    }
}

void Psg::write_nr42(uint8_t v) noexcept
{
    if(sound_enabled)
    {
        nr42 = v;
        c4.check_dac();
        c4.env_write(v);
    }
}

uint8_t Psg::read_nr42() const noexcept
{
    return nr42;
}

void Psg::write_nr43(uint8_t v) noexcept
{
    if(sound_enabled)
    {
        nr43 = v;
        c4.noise_write(v);
    }
}

uint8_t Psg::read_nr43() const noexcept
{
    return nr43;
}


void Psg::write_nr44(uint8_t v) noexcept
{
    if(sound_enabled)
    {
        if(is_set(v,7)) // trigger
        {
            c4.length_trigger();
            c4.env_trigger();
            c4.noise_trigger();
        }

        c4.length_write(v);		
        nr44 = v | 63;

        c4.check_dac();	
    }
}

uint8_t Psg::read_nr44() const noexcept
{
    return (nr44 & (64)) | (0xff-64);	
}




void Psg::write_nr50(uint8_t v) noexcept
{
    if(sound_enabled)
    {
        nr50 = v;
    }
}

uint8_t Psg::read_nr50() const noexcept
{
    return nr50;
}
    
void Psg::write_nr51(uint8_t v) noexcept
{
    if(sound_enabled)
    {
        nr51 = v;
    }
}

uint8_t Psg::read_nr51() const noexcept
{
    return nr51;
}



uint8_t Psg::read_nr52() const noexcept
{
    return nr52;
}


void Psg::nr1_freq_writeback(uint16_t v) noexcept
{
    // write back low 8
    nr13 = v & 0xff;

    
    // and high 3
    nr14 &= ~0x7; // mask bottom 3
    nr14 |= (v >> 8) & 0x7; // and write them out    
}

void Psg::write_wave_table(int idx, uint8_t v) noexcept
{
    assert(idx < 0x20);

    // if wave is on write to current byte <-- finish accuracy later
    if(chan_enabled(2))
    {
        if(is_cgb)
        {
            c3.wave_table[(c3.get_duty_idx() / 2)] = v;
        }
    }

    else // if its off allow "free reign" over it
    {
        c3.wave_table[idx] = v;	
    }
}

uint8_t Psg::read_wave_table(int idx) const noexcept
{
    assert(idx < 0x20);

    // if wave is on write to current byte <-- finish accuracy later
    if(chan_enabled(2))
    {
        // can only access on dmg when the wave channel is...
        // todo
        if(is_cgb)
        {
            return c3.wave_table[c3.get_duty_idx() / 2];
        }

        else
        {
            return 0xff;
        }
    }
    
    // if its off allow "free reign" over it
    return c3.wave_table[idx];    
}


const uint8_t &Psg::get_dac_ref(int idx) const noexcept
{
    assert(idx < 4);
    // //static constexpr uint16_t dac_regs[] = {IO_NR12,IO_NR22,IO_NR30,IO_NR42};	
    switch(idx)
    {
        case 0: return nr12;
        case 1: return nr22;
        case 2: return nr30;
        case 3: return nr42;
    }
    assert(false);
}

void Psg::save_state(std::ofstream &fp)
{
	file_write_var(fp,is_cgb);


	file_write_var(fp,sound_enabled);

	file_write_var(fp,sequencer_step);

	// backing regs

	// nr1x
	file_write_var(fp,nr10);
	file_write_var(fp,nr11);
    file_write_var(fp,nr12);
	file_write_var(fp,nr13);
	file_write_var(fp,nr14);

	// nr2x
    file_write_var(fp,nr21);
	file_write_var(fp,nr22);
	file_write_var(fp,nr23);
	file_write_var(fp,nr24);

	// nr3x
	file_write_var(fp,nr30);
	file_write_var(fp,nr31);
	file_write_var(fp,nr32);
	file_write_var(fp,nr33);
	file_write_var(fp,nr34);

	// nr4x
	file_write_var(fp,nr41);
	file_write_var(fp,nr42);
	file_write_var(fp,nr43);
	file_write_var(fp,nr44);	

	// nr5x
	file_write_var(fp,nr50);
	file_write_var(fp,nr51);
	file_write_var(fp,nr52);
}

void Psg::load_state(std::ifstream &fp)
{
	file_read_var(fp,is_cgb);


	file_read_var(fp,sound_enabled);

	file_read_var(fp,sequencer_step);

	// backing regs

	// nr1x
	file_read_var(fp,nr10);
	file_read_var(fp,nr11);
    file_read_var(fp,nr12);
	file_read_var(fp,nr13);
	file_read_var(fp,nr14);

	// nr2x
    file_read_var(fp,nr21);
	file_read_var(fp,nr22);
	file_read_var(fp,nr23);
	file_read_var(fp,nr24);

	// nr3x
	file_read_var(fp,nr30);
	file_read_var(fp,nr31);
	file_read_var(fp,nr32);
	file_read_var(fp,nr33);
	file_read_var(fp,nr34);

	// nr4x
	file_read_var(fp,nr41);
	file_read_var(fp,nr42);
	file_read_var(fp,nr43);
	file_read_var(fp,nr44);	

	// nr5x
	file_read_var(fp,nr50);
	file_read_var(fp,nr51);
	file_read_var(fp,nr52);
}

}