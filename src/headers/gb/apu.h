#pragma once
#include <destoer-emu/lib.h>
#include <frontend/gb/playback.h>
#include <gb/forward_def.h>
#include <gb/mem_constants.h>
#include <gb/scheduler.h>

namespace gameboy_psg
{
struct Psg;

enum psg_mode
{
	dmg,
	cgb,
	gba
};

// store common stuff then just store NOISE and sweep seperately
struct Channel
{
	int output = 0;
	bool enabled = false;

	// length counters
	int lengthc = 0;
	bool length_enabled = false;
	int max_len = 0;
	int len_mask = 0;

	// frequency
	int freq = 0;
	int freq_lower_mask = 0;
	int period = 0;
	int period_scale = 0;
	int period_factor = 0;

	// vol 
	int volume = 0;
	int volume_load;

	// envelope
	int env_period = 0;
	int env_load = 0;
	bool env_enabled = 0;
	bool env_up = true;

	// duty
	int cur_duty = 0;
	int duty_idx = 0;

	bool dac_on = false;
};


// channel
void init_channels(psg_mode mode, Channel *channels);
void tick_length_counters_internal(Channel *channels);
void disable_chan(Channel &c);
void reset_length(Channel &c);
void length_trigger(Channel &c);
void write_lengthc(Channel &c, u8 v);
void length_write(Channel &c, u8 v, u8 seq_step);

// freq
void freq_reload_period(Channel &c);
void freq_trigger(Channel &c);
void freq_write_higher(Channel &c, u8 v);
void freq_write_lower(Channel &c, u8 v);

// env
void clock_envelope(Channel &c);
void env_write(Channel &c, u8 v); 
void env_trigger(Channel &c);
// dac
void check_dac(Channel &c);
static constexpr int dac_masks[] = {248,248,128,248};


// square
bool square_tick_period(Channel &c,u32 cycles);
void duty_trigger(Channel &c);
void write_cur_duty(Channel &c, u8 v);

// sweep
struct Sweep
{
	bool enabled;
	int shadow = 0;
	int period = 0;
	int timer = 0;
	int calced = false;
    int reg = 0;  	
};

void clock_sweep(Psg &psg);
void init_sweep(Sweep &sweep);
void sweep_write(Sweep &s, Channel &c,u8 v);
void sweep_trigger(Sweep &s, Channel &c);


// noise
struct Noise
{
	int clock_shift = 0;
	int counter_width = 0;
	int divisor_idx = 0; // indexes into divisors table
	uint16_t shift_reg = 0; // 15 bit reg
};

void init_noise(Noise &noise);
void noise_write(Noise &n,uint8_t v);
void noise_trigger(Noise &n);
void noise_reload_period(Channel &c,Noise &n);
bool noise_tick_period(Noise &n,Channel &c, u32 cycles);

struct Wave
{
	u8 table[2][0x20];
	bool bank_idx = false;
	bool dimension = false;
	psg_mode mode;
};

void init_wave(Wave &w, psg_mode mode);
void wave_write_vol(Channel &c, u8 v);
void wave_vol_trigger(Channel &c);
bool wave_tick_period(Wave &w, Channel &c, u32 cycles);
void wave_trigger(Channel &c);


struct Psg
{
	Psg();
	void init(psg_mode mode,bool use_bios);

	void reset_sequencer() noexcept;
	void advance_sequencer() noexcept;
	void tick_periods(uint32_t cycles) noexcept;
	void enable_sound() noexcept;
	void disable_sound() noexcept;

	void save_state(std::ofstream &fp);
	void load_state(std::ifstream &fp);

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


	Channel channels[4];
	Sweep sweep;
	Noise noise;
	Wave wave;


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

	psg_mode mode;

	void tick_length_counters() noexcept;
	void clock_envelopes() noexcept;

	

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

}

namespace gameboy
{


struct Apu
{	
	Apu(GB &gb);

	void insert_new_sample_event() noexcept;

	void push_samples(uint32_t cycles) noexcept;

	void init(gameboy_psg::psg_mode mode, bool use_bios) noexcept;

	void tick(uint32_t cycles) noexcept;

	void disable_sound() noexcept;
	void enable_sound() noexcept;

	void save_state(std::ofstream &fp);
	void load_state(std::ifstream &fp);

	void insert_chan1_period_event()
	{
		insert_period_event(psg.channels[0].period,gameboy_event::c1_period_elapse);
	}

	void insert_chan2_period_event()
	{
		insert_period_event(psg.channels[1].period,gameboy_event::c2_period_elapse);
	}

	void insert_chan3_period_event()
	{
		insert_period_event(psg.channels[2].period,gameboy_event::c3_period_elapse);
	}

	void insert_chan4_period_event()
	{
		insert_period_event(psg.channels[3].period,gameboy_event::c4_period_elapse);
	}

	gameboy_psg::Psg psg;
	GbPlayback playback;

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