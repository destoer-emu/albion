#pragma once
#include <albion/lib.h>
#include <albion/debug.h>
#include <frontend/playback.h>
#include <gba/forward_def.h>
#include <gba/apu_io.h>
#include <gb/apu.h>
#include <gba/scheduler.h>

namespace gameboyadvance
{
    
struct Apu
{
    Apu(GBA &gba);

    void init();
    void tick(int cylces); 
    void push_samples(int cycles);

    void push_dma_a(int8_t x);
    void push_dma_b(int8_t x);

	void disable_sound();
	void enable_sound();

    void insert_new_sample_event();


    void insert_sequencer_event()
    {
        const auto event = scheduler.create_event((16*1024*1024) / 512,gba_event::psg_sequencer);
        scheduler.insert(event,false);
    }

	void insert_chan1_period_event()
	{
		insert_period_event(psg.channels[0].period,gba_event::c1_period_elapse);
	}

	void insert_chan2_period_event()
	{
		insert_period_event(psg.channels[1].period,gba_event::c2_period_elapse);
	}

	void insert_chan3_period_event()
	{
		insert_period_event(psg.channels[2].period,gba_event::c3_period_elapse);
	}

	void insert_chan4_period_event()
	{
		insert_period_event(psg.channels[3].period,gba_event::c4_period_elapse);
	}

	void insert_period_event(int period, gba_event chan) noexcept
	{
		// create  a new event as the period has changed
		// need to half the ammount if we are in double speed
		const auto event = scheduler.create_event(period,chan);

		// dont tick off the old event as 
		// it will use the new value as we have just overwritten 
		// the old internal counter
		// this is not an event we are dropping and expecting to start later 

		scheduler.insert(event,false);
	}


    ApuIo apu_io;

    Playback playback;
    gameboy_psg::Psg psg;

    Mem &mem;
    Cpu &cpu;
    GBAScheduler &scheduler;

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