#include <gb/gb.h>
#include <albion/audio.h>

namespace gameboy
{

Apu::Apu(GB &gb) : scheduler(gb.scheduler)
{
    // init our audio playback
    audio_buffer = make_audio_buffer();
}

void Apu::init(gameboy_psg::psg_mode mode, bool use_bios) noexcept
{
    reset_audio_buffer(audio_buffer);
    psg.init(mode,use_bios);

	enable_sound();

    down_sample_cnt = DOWN_SAMPLE_LIMIT;

    insert_new_sample_event(); 
}

void Apu::tick(u32 cycles) noexcept
{
    if(!psg.sound_enabled)
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

void Apu::push_samples(u32 cycles) noexcept
{
	// handle audio output 
    down_sample_cnt -= cycles;
	if(down_sample_cnt <= 0)
	{
		down_sample_cnt += DOWN_SAMPLE_LIMIT;
        insert_new_sample_event();

        f32 output[4];
        for(int i = 0; i < 4; i++)
        {
            output[i] = f32(psg.channels[i].output) / 16.0f;
        }


		const auto sound_select = psg.read_nr51();
        const auto nr50 = psg.read_nr50();

        const f32 left = gameboy_psg::mix_psg_channels(output,(nr50 >> 4) & 7,(sound_select >> 4) & 0xf,true);
        const f32 right = gameboy_psg::mix_psg_channels(output,nr50 & 7,sound_select & 0xf,true);

        // push our samples!
        push_sample(audio_buffer,left,right);
    }
}

}