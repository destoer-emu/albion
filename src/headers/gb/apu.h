#pragma once
#include <destoer-emu/lib.h>
#include <frontend/gb/playback.h>
#include <gb/forward_def.h>
#include <gb/mem_constants.h>
#include <gb/scheduler.h>

namespace gameboy
{

class Psg;


class Channel
{
public:
	Channel(int c,Psg &p);

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

	Psg &psg;

    int lengthc = 0;
    bool length_enabled = false;
    const int chan_number; // must be inited

	int output = 0;

	bool length_extra_tick;

	// ideally these should be consts...
	const int max_len;
	const int len_mask;

	const uint8_t &dac_reg;
	const uint16_t dac_mask;

	static constexpr int max_lengths[] = {0x40,0x40,0x100,0x40};
	static constexpr int len_masks[] = {0x3f,0x3f,0xff,0x3f};

	static constexpr int dac_masks[] = {248,248,128,248};
};


class FreqReg
{

public:
	FreqReg(int c);
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
protected:
	int freq = 0;
	int period = 0;


	const int freq_lower_mask;
	const int period_scale;

	// write info
	static constexpr int freq_lower_masks[] = {~0xff,0x700,~0xff};
	static constexpr int freq_period_scales[] = {4,4,2};


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
	Square(int c,Psg &p);
	void init() noexcept;
	bool tick_period(uint32_t cycles) noexcept;
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
	Sweep(int c,Psg &p);
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
	Wave( int c, Psg &p);
	void init(bool is_cgb) noexcept;
	void wave_trigger() noexcept;
	void vol_trigger() noexcept;
	void write_vol(uint8_t v) noexcept;
	bool tick_period(uint32_t cycles) noexcept;
	void save_state(std::ofstream &fp);
	void load_state(std::ifstream &fp);

	uint8_t wave_table[0x20];

private:
	bool is_cgb;
	int volume = 0;
	int volume_load = 0;
};


class Noise : public Channel, public Envelope
{
public:
	Noise(int c,Psg &p);


	void init() noexcept;
	bool tick_period(uint32_t cycles) noexcept;
	void reload_period() noexcept;
	int get_period() const noexcept;
	void noise_write(uint8_t v) noexcept;
	void noise_trigger() noexcept;
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



class Psg
{
public:
	Psg();
	void init(bool is_cgb);

	void reset_sequencer() noexcept;
	int get_sequencer_step() const noexcept;
	void advance_sequencer() noexcept;
	void tick_periods(uint32_t cycles) noexcept;
	void enable_sound() noexcept;
	void disable_sound() noexcept;

	void save_state(std::ofstream &fp);
	void load_state(std::ifstream &fp);

	bool enabled() const noexcept
	{
		return sound_enabled;
	}

	bool chan_enabled(int chan) const noexcept
	{
		return is_set(nr52,chan);
	}

	void disable_chan(int chan_number) noexcept
	{
		nr52 = deset_bit(nr52,chan_number);
	}

	void enable_chan(int chan_number) noexcept
	{
		nr52 = set_bit(nr52,chan_number);    
	}


	const uint8_t& get_dac_ref(int idx) const noexcept;


	Sweep c1;
	Square c2;
	Wave c3;
	Noise c4;

	void write_wave_table(int idx, uint8_t v) noexcept;
	uint8_t read_wave_table(int idx) const noexcept;

	// register access

	// nr1x
	void write_nr10(uint8_t v) noexcept;
	uint8_t read_nr10() const noexcept;
	void nr1_freq_writeback(uint16_t v) noexcept;

	void write_nr11(uint8_t v) noexcept;
	uint8_t read_nr11() const noexcept;

	void write_nr12(uint8_t v) noexcept;
	uint8_t read_nr12() const noexcept;

	void write_nr13(uint8_t v) noexcept;
	// write only

	void write_nr14(uint8_t v) noexcept;
	uint8_t read_nr14() const noexcept;

	// nr2x
	void write_nr21(uint8_t v) noexcept;
	uint8_t read_nr21() const noexcept;

	void write_nr22(uint8_t v) noexcept;
	uint8_t read_nr22() const noexcept;

	void write_nr23(uint8_t v) noexcept;
	// write only

	void write_nr24(uint8_t v) noexcept;
	uint8_t read_nr24() const noexcept;
	
	// nr3x
	void write_nr30(uint8_t v) noexcept;
	uint8_t read_nr30() const noexcept;

	void write_nr31(uint8_t v) noexcept;
	// write only

	void write_nr32(uint8_t v) noexcept;
	uint8_t read_nr32() const noexcept;

	void write_nr33(uint8_t v) noexcept;
	// write only

	void write_nr34(uint8_t v) noexcept;
	uint8_t read_nr34() const noexcept;

	// nr4x

	void write_nr41(uint8_t v) noexcept;
	// write only

	void write_nr42(uint8_t v) noexcept;
	uint8_t read_nr42() const noexcept;

	void write_nr43(uint8_t v) noexcept;
	uint8_t read_nr43() const noexcept;

	void write_nr44(uint8_t v) noexcept;
	uint8_t read_nr44() const noexcept;


	// nr5x
	void write_nr50(uint8_t v) noexcept;
	uint8_t read_nr50() const noexcept;

	void write_nr51(uint8_t v) noexcept;
	uint8_t read_nr51() const noexcept;

	//void write_nr52(uint8_t v) noexcept;
	// ^ handled in the memory function 
	uint8_t read_nr52() const noexcept;

private:
	void tick_length_counters() noexcept;
	void clock_envelopes() noexcept;

	bool is_cgb;


	bool sound_enabled = true;

	int sequencer_step = 0;

	// backing regs

	// nr1x
	uint8_t nr10;
	uint8_t nr11;
	uint8_t nr12;
	uint8_t nr13;
	uint8_t nr14;

	// nr2x
	uint8_t nr21;
	uint8_t nr22;
	uint8_t nr23;
	uint8_t nr24;

	// nr3x
	uint8_t nr30;
	uint8_t nr31;
	uint8_t nr32;
	uint8_t nr33;
	uint8_t nr34;

	// nr4x
	uint8_t nr41;
	uint8_t nr42;
	uint8_t nr43;
	uint8_t nr44;	

	// nr5x
	uint8_t nr50;
	uint8_t nr51;
	uint8_t nr52;
};

class Apu
{	
public:
	Apu(GB &gb);

	void insert_new_sample_event() noexcept;

	void push_samples(uint32_t cycles) noexcept;

	void init(bool is_cgb) noexcept;

	void tick(uint32_t cycles) noexcept;

	void disable_sound() noexcept;
	void enable_sound() noexcept;

	void save_state(std::ofstream &fp);
	void load_state(std::ifstream &fp);

	void insert_chan1_period_event()
	{
		insert_period_event(psg.c1.get_period(),gameboy_event::c1_period_elapse);
	}

	void insert_chan2_period_event()
	{
		insert_period_event(psg.c2.get_period(),gameboy_event::c2_period_elapse);
	}

	void insert_chan3_period_event()
	{
		insert_period_event(psg.c3.get_period(),gameboy_event::c3_period_elapse);
	}

	void insert_chan4_period_event()
	{
		insert_period_event(psg.c4.get_period(),gameboy_event::c4_period_elapse);
	}

	Psg psg;
	GbPlayback playback;
private:
	// needs to go in with the psg class
	bool is_cgb;

	void insert_period_event(int period, gameboy_event chan) noexcept
	{
		// create  a new event as the period has changed
		// need to half the ammount if we are in double speed
		const auto event = scheduler.create_event(period << scheduler.is_double(),chan);

		// dont tick off the old event as 
		// it will use the new value as we have just overwritten 
		// the old internal counter
		// this is not an event we are dropping and expecting to start later 

		scheduler.insert(event,false);
	}

	GameboyScheduler &scheduler;

	// counter used to down sample	
	int down_sample_cnt = 0; 


	static constexpr int freq_playback = 44100;
	static constexpr int down_sample_lim = (4 * 1024 * 1024) / freq_playback;

};

}