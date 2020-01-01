#pragma once
#include "lib.h"
#include "forward_def.h"
#include "mem_constants.h"

/*
// should have a bool for the channel being enabled
// as it is better to cache it
typedef struct 
{
	int lengthc;
	bool length_enabled;
	int period; // how long before we move to the next sample
	float output; // output of the channel
	int volume; // should have envelope regs here
	int volume_load;
	int duty; // current duty 
	int duty_idx; // current index into duty
	int freq; // current frequency
	int env_period; // current timer
	int env_load; // cached period
	bool env_enabled; // disabled when it ticks over or under
} Sqaure;



static const uint8_t duty[4][8] = 
{
    {0,0,0,0,0,0,0,1},    // 12.5
    {1,0,0,0,0,0,0,1},   // 25
    {1,0,0,0,0,1,1,1},   // 50 
    {0,1,1,1,1,1,1,0}    // 75
};

//http://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware#Noise_Channel
static const int divisors[8] = { 8, 16, 32, 48, 64, 80, 96, 112 };

*/


class Channel
{
public:
	void init_channel(Memory *mem, int chan_number);

	// length counter
    void tick_lengthc();
	void length_trigger(uint8_t v, int sequencer_step);
	void enable_lengthc();
	void set_lengthc(int c);

	void enable_chan();
	void disable_chan();

	void write_lengthc(uint8_t v);
	void check_dac();


protected:
    int lengthc = 0;
    bool length_enabled = false;
    int chan_number; // must be inited
	int period = 0;
	int volume = 0;

	Memory *mem;

	uint16_t trigger_addr;
	int max_len;
	int len_mask;

	uint16_t dac_reg;
	uint16_t dac_mask;

	static constexpr uint16_t trigger_regs[] = {IO_NR14,IO_NR24,IO_NR34,IO_NR44};
	static constexpr int max_lengths[] = {0x40,0x40,0x100,0x40};
	static constexpr int len_masks[] = {0x3f,0x3f,0xff,0x3f};

	static constexpr uint16_t dac_regs[] = {IO_NR12,IO_NR22,IO_NR30,IO_NR42};	
	static constexpr uint16_t dac_masks[] = {248,248,128,248};
};


// sqaure is same as sweep but lacks the freq sweep
class Sqaure : public Channel
{
public:
};

class Sweep : public Sqaure
{
public:
};

class Wave : public Channel
{
public:
};


class Noise : public Channel
{
public:
};

class Apu
{
public:
	void init(Memory *mem);

	

	void disable_sound();
	void enable_sound();

	void reset_sequencer();
	int get_sequencer_step() const;
	void advance_sequencer();

	bool chan_enabled(int chan);

	bool enabled() const;

	Sweep c1;
	Sqaure c2;
	Wave c3;
	Noise c4;
private:

	void tick_length_counters();
	int sequencer_step = 0;
	Memory *mem;

	bool sound_enabled = true;
};