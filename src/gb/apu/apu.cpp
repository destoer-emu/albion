#include <gb/gb.h>



namespace gameboy
{

Apu::Apu(GB &gb) : scheduler(gb.scheduler)
{
    // init our audio playback
    playback.init(freq_playback,2048);
}

void Apu::init(gameboy_psg::psg_mode mode, bool use_bios) noexcept
{
    psg.init(mode,use_bios);

	enable_sound();


	playback.start();

    down_sample_cnt = down_sample_lim;

    insert_new_sample_event(); 
}

void Apu::tick(uint32_t cycles) noexcept
{
    if(!psg.enabled())
    {
        return;
    }

    psg.tick_periods(cycles);

    push_samples(cycles);
}

void Apu::disable_sound() noexcept
{
    psg.disable_sound();

    // remove all our events for the apu until we renable it
    scheduler.remove(gameboy_event::c1_period_elapse,false);
    scheduler.remove(gameboy_event::c2_period_elapse,false);
    scheduler.remove(gameboy_event::c3_period_elapse,false);
    scheduler.remove(gameboy_event::c4_period_elapse,false);

}

void Apu::enable_sound() noexcept
{
    psg.enable_sound();

    // renable our events in the scheduler
    insert_chan1_period_event();
    insert_chan2_period_event();
    insert_chan3_period_event();
    insert_chan4_period_event();
}

void Apu::insert_new_sample_event() noexcept
{
    // handle double speed in a min
    const auto event = scheduler.create_event(down_sample_cnt << scheduler.is_double(),gameboy_event::sample_push);
    scheduler.insert(event,false);
}

void Apu::push_samples(uint32_t cycles) noexcept
{
	// handle audio output 
    down_sample_cnt -= cycles;
	if(down_sample_cnt <= 0)
	{
		down_sample_cnt = down_sample_lim;
        insert_new_sample_event();

		if(!playback.is_playing()) 
		{ 
			return; 
		}

	
		// left output
        float output[4];
        output[0] = static_cast<float>(psg.c1.get_output()) / 100;
        output[1] = static_cast<float>(psg.c2.get_output()) / 100;
        output[2] = static_cast<float>(psg.c3.get_output()) / 100;
        output[3] = static_cast<float>(psg.c4.get_output()) / 100;

		const auto sound_select = psg.read_nr51();
        const auto nr50 = psg.read_nr50();

        // mix left and right channels
        float f0 = 0;
        int volume = 16*((nr50 & 7)+1);
        for(int i = 0; i < 4; i++)
        {
            if(is_set(sound_select,i))
            {
                const float f1 = output[i];
                playback.mix_samples(f0,f1,volume);
            }            
        }
        const float left = f0;

		// right output
		f0 = 0.0f;
        volume = 16*(((nr50 >> 4) & 7)+1);
        for(int i = 0; i < 4; i++)
        {
            if(is_set(sound_select,i+4))
            {
                const float f1 = output[i];
                playback.mix_samples(f0,f1,volume);
            }            
        }
        const float right = f0;


        // push our samples!
        playback.push_sample(left,right);
    }
}

}