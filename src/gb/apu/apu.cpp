#include <gb/gb.h>



namespace gameboy
{

Apu::Apu(GB &gb) : c1{gb,0}, c2{gb,1}, c3{gb,2},c4{gb,3}, 
    mem(gb.mem), scheduler(gb.scheduler)
{
    // init our audio playback
    playback.init(freq_playback,2048);
}

void Apu::init() noexcept
{
    // init every channel
    c1.init(); c1.sweep_init();
    c2.init();
    c3.init();
    c4.init();

	sequencer_step = 0;
	enable_sound();


	playback.start();

    down_sample_cnt = down_sample_lim;
}



void Apu::advance_sequencer() noexcept
{
	// go to the next step
	sequencer_step = (sequencer_step + 1) & 7;
		

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
}

void Apu::clock_envelopes() noexcept
{
    c1.clock_envelope();
    c2.clock_envelope();
    c4.clock_envelope();
}

void Apu::tick(int cycles) noexcept
{
    if(!enabled())
    {
        return;
    }

    // handled by scheduler
/*
    c1.tick_period(cycles);
    c2.tick_period(cycles);
    c3.tick_period(cycles);
    c4.tick_period(cycles);
*/
    
    push_samples(cycles);
}

void Apu::tick_length_counters() noexcept
{
    c1.tick_lengthc();
    c2.tick_lengthc();
    c3.tick_lengthc();
    c4.tick_lengthc();
}

void Apu::disable_sound() noexcept
{
    // set nr10-nr51 regs to 0
    for(int i = 0x10; i < 0x26; i++)
    {
        mem.write_io(i,0);	
    } 
    
    mem.io[IO_NR52] = 112; // need to write the unused bits and just zero everything else

    // now lock writes
    sound_enabled = false;  

    // remove all our events for the apu until we renable it
    scheduler.remove(event_type::c1_period_elapse);
    scheduler.remove(event_type::c2_period_elapse);
    scheduler.remove(event_type::c3_period_elapse);
    scheduler.remove(event_type::c4_period_elapse);

}

void Apu::enable_sound() noexcept
{
    sound_enabled = true;
    mem.io[IO_NR52] |= 0x80; // data had 0x80 so write back  

    // reset length coutners when powerd up
    // if on cgb
    if(mem.rom_cgb_enabled())
    {
        c1.reset_length();
        c2.reset_length();
        c3.reset_length();
        c4.reset_length();
    }


    // renable our events in the scheduler
    const auto event_c1 = scheduler.create_event(c1.get_period(),event_type::c1_period_elapse);
    const auto event_c2 = scheduler.create_event(c2.get_period(),event_type::c2_period_elapse);
    const auto event_c3 = scheduler.create_event(c3.get_period(),event_type::c3_period_elapse);
    const auto event_c4 = scheduler.create_event(c3.get_period(),event_type::c4_period_elapse);

    scheduler.insert(event_c1);
    scheduler.insert(event_c2);
    scheduler.insert(event_c3);
    scheduler.insert(event_c4);   
}

void Apu::reset_sequencer() noexcept
{
    sequencer_step = 0;
}

int Apu::get_sequencer_step() const noexcept
{
    return sequencer_step;
}

bool Apu::chan_enabled(int chan) const noexcept
{
    return is_set(mem.io[IO_NR52],chan);
}

bool Apu::enabled() const noexcept
{
    return sound_enabled;
}


void Apu::push_samples(int cycles) noexcept
{
	// handle audio output 
    down_sample_cnt -= cycles;
	if(down_sample_cnt <= 0)
	{
		// any cycles that "spill over" should get added
        // down_sample_cnt is negative if this happens
		down_sample_cnt = down_sample_lim + down_sample_cnt;

		if(!playback.is_playing()) 
		{ 
			return; 
		}

		
        float left = 0;
        float right = 0;

		
		// left output
		

        float output[4];
        output[0] = static_cast<float>(c1.get_output()) / 100;
        output[1] = static_cast<float>(c2.get_output()) / 100;
        output[2] = static_cast<float>(c3.get_output()) / 100;
        output[3] = static_cast<float>(c4.get_output()) / 100;

		uint8_t sound_select = mem.io[IO_NR51];

        // mix left and right channels
        float bufferin0 = 0;
        int volume = 20*((mem.io[IO_NR50] & 7)+1);
        for(int i = 0; i < 4; i++)
        {
            if(is_set(sound_select,i))
            {
                float bufferin1 = output[i];
                playback.mix_samples(bufferin0,bufferin1,volume);
            }            
        }
        left = bufferin0;

		// right output
		bufferin0 = 0;
        volume = 20*(((mem.io[IO_NR50] >> 4) & 7)+1);
        for(int i = 0; i < 4; i++)
        {
            if(is_set(sound_select,i+4))
            {
                float bufferin1 = output[i];
                playback.mix_samples(bufferin0,bufferin1,volume);
            }            
        }
        right = bufferin0;


        // push our samples!
        playback.push_sample(left,right);
    }
}

}