#pragma once
#include <destoer-emu/lib.h>
#include "forward_def.h"
#include "mem_constants.h"
#define SDL_MAIN_HANDLED
#ifdef _WIN32
#include <SDL.H>
#else
#include <SDL2/SDL.h>
#endif


class Channel
{
public:
	// length counter
    void tick_lengthc();
	void length_trigger();
	void length_write(uint8_t v, int sequencer_step);

	void enable_chan();
	void disable_chan();
	void check_dac();

	void write_lengthc(uint8_t v);
	int get_output() const;

	void chan_save_state(std::ofstream &fp);
	void chan_load_state(std::ifstream &fp);
protected:
	void init_channel(Memory *m, int chan_number);
	bool dac_on() const;
	bool enabled() const;

    int lengthc = 0;
    bool length_enabled = false;
    int chan_number; // must be inited

	int output = 0;

	Memory *mem;

	// ideally these should be consts...
	uint16_t trigger_addr;
	int max_len;
	int len_mask;

	uint16_t dac_reg;
	uint16_t dac_mask;

	static constexpr uint16_t trigger_regs[] = {IO_NR14,IO_NR24,IO_NR34,IO_NR44};
	static constexpr int max_lengths[] = {0x40,0x40,0x100,0x40};
	static constexpr int len_masks[] = {0x3f,0x3f,0xff,0x3f};

	static constexpr uint16_t dac_regs[] = {IO_NR12,IO_NR22,IO_NR30,IO_NR42};	
	static constexpr int dac_masks[] = {248,248,128,248};
};


class FreqReg
{

public:
	void freq_init();
	void freq_write_lower(uint8_t v);
	void freq_write_higher(uint8_t v);
	void freq_reload_period();
	int get_duty_idx() const;
	void freq_save_state(std::ofstream &fp);
	void freq_load_state(std::ifstream &fp);
protected:
	int freq = 0;
	int period = 0;


	int freq_lower_mask;
	int period_scale;

	// write info
	static constexpr int freq_lower_masks[] = {~0xff,0x700,~0xff};
	static constexpr int freq_period_scales[] = {4,4,2};


	int duty_idx = 0;
};


class Envelope
{
public:
	void env_init();
	void env_trigger();
	void clock_envelope();
	void env_write(uint8_t v);
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
	void init(Memory *mem, int chan_number);
	void tick_period(int cycles);
	void write_cur_duty(uint8_t v);
	void duty_trigger();
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
	void sweep_init();
	void sweep_trigger();
	void sweep_write(uint8_t v);
	uint16_t calc_freqsweep();
	void do_freqsweep();
	void clock_sweep();
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
	void init(Memory *m, int c);
	void wave_trigger();
	void vol_trigger();
	void write_vol(uint8_t v);
	void tick_period(int cycles);
	void save_state(std::ofstream &fp);
	void load_state(std::ifstream &fp);
private:
	int volume = 0;
	int volume_load = 0;
};


class Noise : public Channel, public Envelope
{
public:
	void init(Memory *m, int c);
	void tick_period(int cycles);
	void noise_write(uint8_t v);
	void noise_trigger();
	void save_state(std::ofstream &fp);
	void load_state(std::ifstream &fp);	
private:
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
	void stop_audio();
	void start_audio();


	void init_audio();
	void push_samples();

	void init(Memory *m);

	void tick(int cycles);

	void disable_sound();
	void enable_sound();

	void reset_sequencer();
	int get_sequencer_step() const;
	void advance_sequencer();

	bool chan_enabled(int chan);

	bool enabled() const;

	void set_double(bool d);


	void save_state(std::ofstream &fp);
	void load_state(std::ifstream &fp);

	Sweep c1;
	Square c2;
	Wave c3;
	Noise c4;
private:

	void tick_length_counters();
	void clock_envelopes();
	int sequencer_step = 0;
	Memory *mem;

	bool sound_enabled = true;

	bool play_audio = true;

	bool is_double = false;

	// sound playback
	static constexpr int sample_size = 2048;

	// SDL SOUND
	SDL_AudioSpec audio_spec;
	float audio_buf[sample_size] = {0};
	int audio_buf_idx = 0; // how filled up is the buffer
	int down_sample_cnt = 0; // counter used to down sample	
	bool audio_setup = false;
};
