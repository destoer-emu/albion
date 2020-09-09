#include <gba/gba.h>

namespace gameboyadvance
{

Apu::Apu(GBA &gba) : mem(gba.mem), cpu(gba.cpu)
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
}

void Apu::tick(int cycles)
{
    // should probably tick psgs here

    push_samples(cycles);
}

// down sample it first
// shoud sample at 16mhz / sample rate in sound bias
void Apu::push_samples(int cycles)
{
    down_sample_cnt -= cycles;

    if(down_sample_cnt >= 0)
    {
        return;
    }

    else
    {
        down_sample_cnt = (16 * 1024 * 1024) / 44100;
    }


    if(!playback.is_playing()) 
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
    int volume = 50;


    // mix left and right channels
    float bufferin0 = 0.0;
    float bufferin1 = static_cast<float>(dma_a_sample) / 128.0;
    playback.mix_samples(bufferin0,bufferin1,volume);
    bufferin1 = static_cast<float>(dma_b_sample) / 128.0;
    playback.mix_samples(bufferin0,bufferin1,volume);
    const float left =  bufferin0;

    // right output
    bufferin0 = 0;
    bufferin1 = static_cast<float>(dma_a_sample) / 128.0;
    playback.mix_samples(bufferin0,bufferin1,volume);
    bufferin1 = static_cast<float>(dma_b_sample) / 128.0;
    playback.mix_samples(bufferin0,bufferin1,volume);
    const float right =  bufferin0;

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
