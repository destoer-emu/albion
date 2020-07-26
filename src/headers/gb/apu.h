#pragma once
#include <destoer-emu/lib.h>
#include <frontend/gb/playback.h>
#include <gb/forward_def.h>
#include <gb/mem_constants.h>
#include <gb/scheduler.h>

namespace gameboy
{

class Channel
{
public:
	Channel(GB &gb, int c);

	void init_channel() noexcept;
	// length counter
    void tick_lengthc() noexcept;
	void length_trigger() noexcept;
	void length_write(uint8_t v) noexcept;

	void reset_length() noexcept;

	void enable_chan() noexcept;
	void disable_chan() noexcept;
	void check_dac() noexcept;

	void write_lengthc(uint8_t v) noexcept;
	int get_output() const noexcept;

	void chan_save_state(std::ofstream &fp);
	void chan_load_state(std::ifstream &fp);
protected:
	void init_channel(int chan_number) noexcept;
	bool dac_on() const noexcept;
	bool enabled() const noexcept;

	Memory &mem;
	Apu &apu;

    int lengthc = 0;
    bool length_enabled = false;
    const int chan_number; // must be inited

	int output = 0;

	bool length_extra_tick;

	// ideally these should be consts...
	const uint16_t trigger_addr;
	const int max_len;
	const int len_mask;

	const uint16_t dac_reg;
	const uint16_t dac_mask;

	static constexpr uint16_t trigger_regs[] = {IO_NR14,IO_NR24,IO_NR34,IO_NR44};
	static constexpr int max_lengths[] = {0x40,0x40,0x100,0x40};
	static constexpr int len_masks[] = {0x3f,0x3f,0xff,0x3f};

	static constexpr uint16_t dac_regs[] = {IO_NR12,IO_NR22,IO_NR30,IO_NR42};	
	static constexpr int dac_masks[] = {248,248,128,248};
};


class FreqReg
{

public:
	FreqReg(GB &gb,int c,EventCallback func);
	void freq_init() noexcept;
	void freq_write_lower(uint8_t v) noexcept;
	void freq_write_higher(uint8_t v) noexcept;
	void freq_reload_period() noexcept;
	int get_duty_idx() const noexcept;
	void reset_duty() noexcept;
	void freq_save_state(std::ofstream &fp);
	void freq_load_state(std::ifstream &fp);
	void freq_trigger() noexcept;
	int get_period() const noexcept;

	const EventCallback period_callback;
protected:
	int freq = 0;
	int period = 0;


	const int freq_lower_mask;
	const int period_scale;

	// write info
	static constexpr int freq_lower_masks[] = {~0xff,0x700,~0xff};
	static constexpr int freq_period_scales[] = {4,4,2};


	Scheduler &scheduler;

	static constexpr event_type channel_events[] =
	{
		event_type::c1_period_elapse,
		event_type::c2_period_elapse,
		event_type::c3_period_elapse
	};

	const event_type channel_event;

	int duty_idx = 0;
};


class Envelope
{
public:
	void env_init() noexcept;
	void env_trigger() noexcept;
	void clock_envelope() noexcept;
	void env_write(uint8_t v) noexcept;
	void env_save_state(std::ofstream &fp);
	void env_load_state(std::ifstream &fp);
protected:
	int env_period = 0; // current timer
	int env_load = 0; // cached period
	int volume = 0;
	int volume_load = 0;
	bool env_enabled = false; // disabled when it ticks over or under
	bool env_up = true;
};


// sqaure is same as sweep but lacks the freq sweep
class Square : public Channel, public FreqReg, public Envelope
{
public:
	Square(GB &gb,int c,EventCallback func);
	void init() noexcept;
	void tick_period(int cycles) noexcept;
	void write_cur_duty(uint8_t v) noexcept;
	void duty_trigger() noexcept;
	void save_state(std::ofstream &fp);
	void load_state(std::ifstream &fp);
protected:
	int cur_duty = 0;

	static constexpr int duty[4][8] = 
	{
		{0,0,0,0,0,0,0,1},   // 12.5
		{1,0,0,0,0,0,0,1},   // 25
		{1,0,0,0,0,1,1,1},   // 50 
		{0,1,1,1,1,1,1,0}    // 75
	};
};

class Sweep : public Square
{
public:
	Sweep(GB &gb, int c,EventCallback func);
	void sweep_init() noexcept;
	void sweep_trigger() noexcept;
	void sweep_write(uint8_t v) noexcept;
	uint16_t calc_freqsweep() noexcept;
	void do_freqsweep() noexcept;
	void clock_sweep() noexcept;
	void sweep_save_state(std::ofstream &fp);
	void sweep_load_state(std::ifstream &fp);
private:
	bool sweep_enabled = false;
	uint16_t sweep_shadow = 0;
	int sweep_period = 0;
	int sweep_timer = 0;
	bool sweep_calced = false;
	uint8_t sweep_reg = 0; // nr10 copy
};

class Wave : public Channel, public FreqReg
{
public:
	Wave(GB &gb, int c,EventCallback func);
	void init() noexcept;
	void wave_trigger() noexcept;
	void vol_trigger() noexcept;
	void write_vol(uint8_t v) noexcept;
	void tick_period(int cycles) noexcept;
	void save_state(std::ofstream &fp);
	void load_state(std::ifstream &fp);
private:
	int volume = 0;
	int volume_load = 0;
};


class Noise : public Channel, public Envelope
{
public:
	Noise(GB &gb, int c,EventCallback func);


	void init() noexcept;
	void tick_period(int cycles) noexcept;
	void reload_period() noexcept;
	void noise_write(uint8_t v) noexcept;
	void noise_trigger() noexcept;
	void save_state(std::ofstream &fp);
	void load_state(std::ifstream &fp);	

	const EventCallback period_callback;
private:

	Scheduler &scheduler;

	//http://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware#Noise_Channel
	static constexpr int divisors[8] = { 8, 16, 32, 48, 64, 80, 96, 112 };

	int clock_shift = 0;
	int counter_width = 0;
	int divisor_idx = 0; // indexes into divisors table
	uint16_t shift_reg = 0; // 15 bit reg
	int period = 0;
};

class Apu
{	
public:
	Apu(GB &gb);

	void push_samples(int cycles) noexcept;

	void init() noexcept;

	void tick(int cycles) noexcept;

	void disable_sound() noexcept;
	void enable_sound() noexcept;

	void reset_sequencer() noexcept;
	int get_sequencer_step() const noexcept;
	void advance_sequencer() noexcept;

	bool chan_enabled(int chan) const noexcept;

	bool enabled() const noexcept;

	void save_state(std::ofstream &fp);
	void load_state(std::ifstream &fp);


	// constructors called in apu.cpp
	Sweep c1;
	Square c2;
	Wave c3;
	Noise c4;

	GbPlayback playback;
private:

	Memory &mem;
	Scheduler &scheduler;

	void tick_length_counters() noexcept;
	void clock_envelopes() noexcept;
	int sequencer_step = 0;

	bool sound_enabled = true;

	// counter used to down sample	
	int down_sample_cnt = 0; 


	static constexpr int freq_playback = 44100;
	static constexpr int down_sample_lim = (4 * 1024 * 1024) / freq_playback;

};

}