#pragma once
#include <destoer-emu/lib.h>
#include <destoer-emu/debug.h>
#include <frontend/gba/playback.h>
#include <gba/forward_def.h>
#include <gba/apu_io.h>

namespace gameboyadvance
{
    
class Apu
{
public:
    Apu(GBA &gba);

    void init();
    void tick(int cylces); 

    void push_dma_a(int8_t x);
    void push_dma_b(int8_t x);
    

    ApuIo apu_io;

    GbaPlayback playback;
private:
    void push_samples(int cycles);

    Mem &mem;
    Cpu &cpu;

    int8_t dma_a_sample;
    int8_t dma_b_sample;

    
	// sound playback
	static constexpr int sample_size = 2048;

	//  internal sound playback
	float audio_buf[sample_size] = {0};
    int audio_buf_idx = 0;
    int down_sample_cnt = 380;
};

}