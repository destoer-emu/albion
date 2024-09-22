#include <gba/gba.h>

namespace gameboyadvance
{

Apu::Apu(GBA &gba) : mem(gba.mem), cpu(gba.cpu), scheduler(gba.scheduler)
{
    // init our audio playback
    audio_buffer = make_audio_buffer();
}

void Apu::init()
{
    apu_io.init();

    down_sample_cnt = (16 * 1024 * 1024) / AUDIO_BUFFER_SAMPLE_RATE;
    dma_a_sample = 0;
    dma_b_sample = 0;

    psg.init(gameboy_psg::psg_mode::gba,true);

    insert_new_sample_event();
    insert_sequencer_event();

    enable_sound();
}

void Apu::disable_sound()
{
    psg.disable_sound();


    // remove all our events for the apu until we renable it
    scheduler.remove(gba_event::c1_period_elapse,false);
    scheduler.remove(gba_event::c2_period_elapse,false);
    scheduler.remove(gba_event::c3_period_elapse,false);
    scheduler.remove(gba_event::c4_period_elapse,false);


}

void Apu::enable_sound()
{
    psg.enable_sound();


    // renable our events in the scheduler
    insert_chan1_period_event();
    insert_chan2_period_event();
    insert_chan3_period_event();
    insert_chan4_period_event();


}



void Apu::tick(int cycles)
{

    static uint32_t seq_cycles = 0;

    seq_cycles += cycles;

    if(seq_cycles >= (16 * 1024 * 1024) / 512)
    {
        psg.advance_sequencer();
        seq_cycles = 0;
    }


    if(psg.sound_enabled)
    {
        psg.tick_periods(cycles);   
    }

    push_samples(cycles);
}

void Apu::insert_new_sample_event()
{
    // handle double speed in a min
    const auto event = scheduler.create_event(down_sample_cnt,gba_event::sample_push);
    scheduler.insert(event,false);
}


// down sample it first
// shoud sample at 16mhz / sample rate in sound bias
void Apu::push_samples(int cycles)
{
    down_sample_cnt -= cycles;

    if(down_sample_cnt > 0)
    {
        return;
    }

    else
    {
        down_sample_cnt = ((16 * 1024 * 1024) / AUDIO_BUFFER_SAMPLE_RATE);
        insert_new_sample_event();
    }


    if(!psg.sound_enabled) 
    { 
        return; 
    }

    
    //printf("%d:%d\n",dma_a_sample,dma_b_sample);
    

    // we also need to handle soundbias
    // and eventually the internal resampling rate
    // along with the psg sound scaling
    // figure out how the volume and the bias works properly


    float output[4];
    for(int i = 0; i < 4; i++)
    {
        output[i] = static_cast<float>(psg.channels[i].output) / 100;
    }


    const auto sound_select = psg.read_nr51();
    const auto nr50 = psg.read_nr50();

    const f32 psg_left = gameboy_psg::mix_psg_channels(output,(nr50 >> 4) & 7,(sound_select >> 4) & 0xf,true);
    const f32 psg_right =  gameboy_psg::mix_psg_channels(output,nr50 & 7,sound_select & 0xf,true);

    f32 left = psg_left;

    if(apu_io.sound_cnt.enable_left_a)
    {
        left += f32(dma_a_sample) / 128.0f;
    }

    if(apu_io.sound_cnt.enable_left_b)
    {
        left += f32(dma_b_sample) / 128.0f;
    }

    f32 right = psg_right;

    if(apu_io.sound_cnt.enable_right_a)
    {
        right += f32(dma_a_sample) / 128.0f;
    }

    if(apu_io.sound_cnt.enable_right_b)
    {
        right += f32(dma_b_sample) / 128.0f;
    }

    push_sample(audio_buffer,left,right);
}


void Apu::push_dma_a(int8_t x)
{
    dma_a_sample = x;
}

void Apu::push_dma_b(int8_t x)
{
    dma_b_sample = x;
}

}
