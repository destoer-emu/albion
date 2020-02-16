#include <gb/apu.h>
#include <gb/memory.h>
 
namespace gameboy
{

void Apu::init(Memory *m) noexcept
{
    mem = m;

    // init every channel
    c1.init(mem,0); c1.sweep_init();
    c2.init(mem,1);
    c3.init(mem,2);
    c4.init(mem,3);

	sequencer_step = 0;
	sound_enabled = true;

    // init our audio playback
    if(!audio_setup)
    {
        init_audio();
    }

    down_sample_cnt = 23;
    audio_buf_idx = 0;
	is_double = false;
	play_audio = true;
}

void Apu::start_audio() noexcept
{
	play_audio = true;
}

void Apu::stop_audio() noexcept
{
	play_audio = false;
	SDL_ClearQueuedAudio(1);
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

    c1.tick_period(cycles);
    c2.tick_period(cycles);
    c3.tick_period(cycles);
    c4.tick_period(cycles);
    

    push_samples();
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
        mem->write_io(i,0);	
    } 
    
    mem->io[IO_NR52] = 112; // need to write the unused bits and just zero everything else

    // now lock writes
    sound_enabled = false;  
}

void Apu::enable_sound() noexcept
{
    sound_enabled = true;
    mem->io[IO_NR52] |= 0x80; // data had 0x80 so write back  
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
    return is_set(mem->io[IO_NR52],chan);
}

bool Apu::enabled() const noexcept
{
    return sound_enabled;
}

void Apu::set_double(bool d) noexcept
{
	is_double = d;
}

void Apu::init_audio() noexcept
{
	memset(&audio_spec,0,sizeof(audio_spec));

	audio_spec.freq = 44100;
	audio_spec.format = AUDIO_F32SYS;
	audio_spec.channels = 2;
	audio_spec.samples = sample_size;	
	audio_spec.callback = NULL; // we will use SDL_QueueAudio()  rather than 
	audio_spec.userdata = NULL; // using a callback :)


    SDL_OpenAudio(&audio_spec,NULL);
	SDL_PauseAudio(0);
}

void Apu::push_samples() noexcept
{
	// handle audio output 
	if(!--down_sample_cnt)
	{
		// while our counts are in T cycles the update function is called 
		// ever M cycle so its 23
		down_sample_cnt = 23; // may need adjusting for m cycles (95)
	
		if(is_double) // if in double speed the function is called twice as often 
		{					// so to adjust the down sample count must be double
			down_sample_cnt *= 2;
		}

		if(!play_audio) 
		{ 
			return; 
		}

		
		float bufferin0 = 0;
		float bufferin1 = 0;
		
		// left output
		int volume = (128 *(mem->io[IO_NR50] & 7)) / 7 ;

        float output[4];
        output[0] = static_cast<float>(c1.get_output()) / 100;
        output[1] = static_cast<float>(c2.get_output()) / 100;
        output[2] = static_cast<float>(c3.get_output()) / 100;
        output[3] = static_cast<float>(c4.get_output()) / 100;

		uint8_t sound_select = mem->io[IO_NR51];

        // mix left and right channels
        for(int i = 0; i < 4; i++)
        {
            if(is_set(sound_select,i))
            {
                bufferin1 = output[i];
                SDL_MixAudioFormat((Uint8*)&bufferin0,(Uint8*)&bufferin1,AUDIO_F32SYS,sizeof(float),volume);
            }            
        }

        audio_buf[audio_buf_idx] = bufferin0;

		// right output
		bufferin0 = 0;
        for(int i = 0; i < 4; i++)
        {
            if(is_set(sound_select,i+4))
            {
                bufferin1 = output[i];
                SDL_MixAudioFormat((Uint8*)&bufferin0,(Uint8*)&bufferin1,AUDIO_F32SYS,sizeof(float),volume);
            }            
        }

        audio_buf[audio_buf_idx+1] = bufferin0;
        audio_buf_idx += 2;
    }

    // push audio to sdl
	if(audio_buf_idx >= sample_size)
	{
		audio_buf_idx = 0;
		

		// legacy interface
		static constexpr SDL_AudioDeviceID dev = 1;
		
		auto buffer_size = (sample_size * sizeof(float));

		// delay execution and let the que drain
		while(SDL_GetQueuedAudioSize(dev) > buffer_size)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}			

	
		if(SDL_QueueAudio(dev,audio_buf,buffer_size) < 0)
		{
			printf("%s\n",SDL_GetError()); exit(1);
		}
    }
}

}