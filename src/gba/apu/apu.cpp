#include <gba/gba.h>

namespace gameboyadvance
{

Apu::Apu(GBA &gba) : mem(gba.mem), cpu(gba.cpu), scheduler(gba.scheduler)
{
    playback.init(44100,sample_size);
}

void Apu::init()
{
    apu_io.init();

    // sound is broken?
	playback.start();
    audio_buf_idx = 0;
    down_sample_cnt = (16 * 1024 * 1024) / 44100;
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


    if(psg.enabled())
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
        down_sample_cnt = ((16 * 1024 * 1024) / 44100);
        insert_new_sample_event();
    }


    if(!playback.is_playing() || !psg.enabled()) 
    { 
        return; 
    }

    
    //printf("%d:%d\n",dma_a_sample,dma_b_sample);
    
    // volume calc
    // (this is nice and jank and doesent handle the output properly but just roll with it for a sec)
    // (also the audio on the one rom that works is earbleeding cause it doesent run the rom at the correct speed)
    // (this code also aint checking if the channels are enabled)
    // we also need to handle soundbias
    // and eventually the internal resampling rate
    // along with the psg sound scaling
    // figure out how the volume and the bias works properly tomorrow lol
    int volume = 50;



    float output[4];
    output[0] = static_cast<float>(psg.c1.get_output()) / 100;
    output[1] = static_cast<float>(psg.c2.get_output()) / 100;
    output[2] = static_cast<float>(psg.c3.get_output()) / 100;
    output[3] = static_cast<float>(psg.c4.get_output()) / 100;

    const auto sound_select = psg.read_nr51();
    const auto nr50 = psg.read_nr50();



    // mix left and right channels

    // mix dma left
    float f0 = 0.0;

    if(apu_io.sound_cnt.enable_left_a)
    {
        const float f1 = static_cast<float>(dma_a_sample) / 128.0;
        playback.mix_samples(f0,f1,volume);
    }

    if(apu_io.sound_cnt.enable_left_b)
    {
        const float f1 = static_cast<float>(dma_b_sample) / 128.0;
        playback.mix_samples(f0,f1,volume);
    }
    
    // mix psg left
    int psg_volume = 20*((nr50 & 7)+1);
    for(int i = 0; i < 4; i++)
    {
        if(is_set(sound_select,i))
        {
            const float f1 = output[i];
            playback.mix_samples(f0,f1,psg_volume);
        }            
    }
    const float left = f0;

    // mix dma right
    f0 = 0.0;
    
    if(apu_io.sound_cnt.enable_right_a)
    {
        const float f1 = static_cast<float>(dma_a_sample) / 128.0;
        playback.mix_samples(f0,f1,volume);
    }

    if(apu_io.sound_cnt.enable_right_b)
    {
        const float f1 = static_cast<float>(dma_b_sample) / 128.0;
        playback.mix_samples(f0,f1,volume);
    }

    // mix psg right
    psg_volume = 16*(((nr50 >> 4) & 7)+1);
    for(int i = 0; i < 4; i++)
    {
        if(is_set(sound_select,i+4))
        {
            const float f1 = output[i];
            playback.mix_samples(f0,f1,psg_volume);
        }            
    } 
    const float right = f0;

    playback.push_sample(left,right);
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
