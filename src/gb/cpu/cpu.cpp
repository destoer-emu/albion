#include<gb/gb.h>
namespace gameboy
{

Cpu::Cpu(GB &gb) : mem(gb.mem), apu(gb.apu), ppu(gb.ppu), 
	scheduler(gb.scheduler), debug(gb.debug), disass(gb.disass)
{
	
}

bool Cpu::get_double() const
{
	return is_double;
}

void Cpu::init(bool use_bios)
{
	write_log(debug,"[INFO] new instance started!");


	is_cgb = mem.rom_cgb_enabled();
	//is_cgb = false;

	// setup regs to skip the bios
	if(!use_bios)
	{
		// set the cgb initial registers 
		if(is_cgb)
		{
			write_af(0x1180); // af = 0x1180;
			b = 0x00; c = 0x00; // bc = 0x0000;
			d = 0xff; e = 0x56; // de = 0xff56;
			h = 0x00; l = 0x0d; // hl = 0x000d;
			sp = 0xfffe;
			pc = 0x0100;
		}

		// dmg
		else 
		{
			// set our initial register state
			write_af(0x01b0); // af = 0x01b0
			b = 0x00; c = 0x13; // bc = 0x0013
			d = 0x00; e = 0xd8; // de = 0x00d8
			h = 0x01; l = 0x4d; // hl = 0x014d
			sp = 0xfffe;
			pc = 0x0100;
		}
	}

	// bios all set to zero
	else
	{
		write_af(0x0000); // af = 0x0000
		b = 0x00; c = 0x00; // bc = 0x0000
		d = 0x00; e = 0x00; // de = 0x0000
		h = 0x00; l = 0x00; // hl = 0x0000
		sp = 0x0000;
		pc = 0x0000;		
	}


	is_double = false;
	if(!use_bios)
	{
    	internal_timer = is_cgb? 0x2fb4 : 0xabc4;
	}

	else
	{
		use_bios = 0;
	}
    
    joypad_state = 0xff;

    // interrupts
	instr_side_effect = instr_state::normal;
    interrupt_enable = false;
	interrupt_fire = false;
	interrupt_req = false;
	halt_bug = false;

	serial_cyc = 0;
	serial_cnt = 0;

	pending_cycles = 0;

	insert_new_timer_event();
}


void Cpu::update_intr_req() noexcept
{
	interrupt_req = mem.io[IO_IF] & mem.io[IO_IE] & 0x1f;
	update_intr_fire();
}


void Cpu::update_intr_fire() noexcept
{
	interrupt_fire = interrupt_req && interrupt_enable;
}

uint8_t Cpu::fetch_opcode() noexcept
{
	// need to fetch this before we do the tick
	// just incase we are executing from somewhere volatile
	const auto opcode = mem.read_mem(pc);

	// at midpoint of instr fetch interrupts are checked
	// and if so the opcode is thrown away and interrupt dispatch started
	cycle_tick_t(2);
	const bool fired = interrupt_fire;
	cycle_tick_t(2);

	if(fired)
	{
		// pc will get decremented triggering oam bug
		// dont know how this plays with halt bug
		oam_bug_write(pc);
		do_interrupts();

		// have to re fetch the opcode this costs a cycle
		const auto v = mem.read_memt(pc);

		// if halt bug occurs pc fails to increment for one instr
		pc = pc + (1 - halt_bug);
		halt_bug = false; 
		return v;
	}

	else // return the opcode we have just fetched
	{
		pc = pc + (1 - halt_bug);
		halt_bug = false; 
		return opcode;
	}
	
}

// cycles that dont need to be ticked yet
// as there is no memory access involed
// takes t cycles
void Cpu::cycle_delay(uint32_t cycles) noexcept
{
	pending_cycles += cycles;
}

void Cpu::tick_pending_cycles() noexcept
{
	if(pending_cycles > 0)
	{
		cycle_tick_t(0);
	}
}


// m cycle tick
void Cpu::cycle_tick(uint32_t cycles) noexcept
{
	//  convert to t cycles and tick
	cycle_tick_t(cycles * 4);
}

// t cycle tick
void Cpu::cycle_tick_t(uint32_t cycles) noexcept
{
	// tick off any cycles that are pending
	// from instr cycles that dont do memory accesses
	cycles += pending_cycles;
	pending_cycles = 0;
/*
	// timers act at constant speed
	update_timers(cycles); 

	// handler will check if its enabled
	mem.tick_dma(cycles);
	
	// in double speed mode gfx and apu should operate at half
	ppu.update_graphics(cycles >> is_double); // handle the lcd emulation

	apu.tick(cycles >> is_double); // advance the apu state	

	tick_serial(cycles);
*/
	scheduler.tick(cycles);

	//apu.tick(cycles >> is_double); // advance the apu state	

	// if we are using the fifo this needs to be ticked each time
	if(ppu.using_fifo())
	{
		ppu.update_graphics(cycles >> is_double); // handle the lcd emulation
	}

}

void Cpu::switch_double_speed() noexcept
{
	puts("double speed");
	
	tick_pending_cycles();

	const bool c1_active = scheduler.is_active(gameboy_event::c1_period_elapse);
	const bool c2_active = scheduler.is_active(gameboy_event::c2_period_elapse);
	const bool c3_active = scheduler.is_active(gameboy_event::c3_period_elapse);
	const bool c4_active = scheduler.is_active(gameboy_event::c4_period_elapse);
	const bool sample_push_active = scheduler.is_active(gameboy_event::sample_push);
	const bool internal_timer_active = scheduler.is_active(gameboy_event::internal_timer);
	const bool ppu_active = scheduler.is_active(gameboy_event::ppu);

	static constexpr std::array<gameboy_event,7> double_speed_events = 
	{
		gameboy_event::sample_push,gameboy_event::internal_timer,
		gameboy_event::c1_period_elapse,gameboy_event::c2_period_elapse,
		gameboy_event::c3_period_elapse,gameboy_event::c4_period_elapse,
		gameboy_event::ppu
	};

	// remove all double speed events so they can be ticked off
	for(const auto &e: double_speed_events)
	{
		scheduler.remove(e);
	}

	is_double = !is_double;


	if(c1_active)
	{
		apu.insert_chan1_period_event();
	}

	if(c2_active)
	{
		apu.insert_chan2_period_event();
	}

	if(c3_active)
	{
		apu.insert_chan3_period_event();
	}

	if(c4_active)
	{
		apu.insert_chan4_period_event();
	}

	if(sample_push_active)
	{
		apu.insert_new_sample_event();
	}

	if(internal_timer_active)
	{
		insert_new_timer_event();
	}

	if(ppu_active)
	{
		ppu.insert_new_ppu_event();
	}


}

void Cpu::tima_reload() noexcept
{
	mem.io[IO_TIMA] = mem.io[IO_TMA]; // reset to value in tma
	// timer overflow interrupt	this happens on the reload
	request_interrupt(2); 
}

void Cpu::tima_inc() noexcept
{
	// timer about to overflow
	if(mem.io[IO_TIMA] == 255)
	{	
		// zero until reloaded 4 cycles later
		mem.io[IO_TIMA] = 0; 

		const auto reload_event = scheduler.create_event(4,gameboy_event::timer_reload);
		scheduler.insert(reload_event,false);		
	}
			
	else
	{
		mem.io[IO_TIMA]++;
	}	
}

bool Cpu::internal_tima_bit_set() const noexcept
{

	const uint8_t freq = mem.io[IO_TMC] & 0x3;
	const int bit = freq_arr[freq];

	return is_set(internal_timer,bit);
}

int Cpu::get_next_timer_event() const noexcept
{
	// find what happens next
	const uint8_t freq = mem.io[IO_TMC] & 0x3;
	const int timer_bit = freq_arr[freq];

	const int sequencer_bit = is_double? 13 : 12;

	
	const int sequencer_lim = 1 << sequencer_bit;
	int seq_cycles = sequencer_lim - (internal_timer & (sequencer_lim - 1));

	// falling edge so if its not set we need to wait
	// for another one to have it set again
	if(!is_set(internal_timer,sequencer_bit))
	{
		seq_cycles += sequencer_lim;	
	}

	// if the tima is inactive then the seq event is next
	if(!tima_enabled())
	{
		return seq_cycles;
	}

	const int timer_lim = 1 << timer_bit;
	int timer_cycles = timer_lim - (internal_timer & (timer_lim - 1));

	if(!is_set(internal_timer,timer_bit))
	{
		timer_cycles += timer_lim;
	}


	return std::min(seq_cycles,timer_cycles);
}

bool Cpu::tima_enabled() const noexcept
{
	return is_set(mem.io[IO_TMC],2);	
}

// insert a new event and dont tick of the old one as its not there
void Cpu::insert_new_timer_event() noexcept
{
	const auto timer_event = scheduler.create_event(get_next_timer_event(),gameboy_event::internal_timer);
	scheduler.insert(timer_event,false); 
}

void Cpu::update_timers(uint32_t cycles) noexcept
{

	// internal timer actions occur on a falling edge
	// however what this means here in practice is that
	// the next bit over from one that falls gets a carry
	// and therefore is no longer its old value
	// we take advantage of this to make calculating event times easier
	// in our memory handlers we are still using the standard bits


	// note bit is + 1 as we are checking the next one over has changed
	const uint8_t freq = mem.io[IO_TMC] & 0x3;
	const int timer_bit = freq_arr[freq]+1;


	const int sound_bit = (is_double? 13 : 12) + 1;

	const bool sound_bit_old = is_set(internal_timer,sound_bit);
	

	// if a falling edge has occured
	// and the timer is enabled of course
	if(tima_enabled())
	{
		const bool timer_bit_old = is_set(internal_timer,timer_bit);

		internal_timer += cycles;

		const bool timer_bit_new = is_set(internal_timer,timer_bit);

		if(timer_bit_new != timer_bit_old)
		{
			tima_inc();
		}
		
		// we repeat this here because we have to add the cycles
		// at the proper time for the falling edge det to work
		// and we dont want to waste time handling the falling edge
		// for the timer when its off
		if(is_set(internal_timer,sound_bit) != sound_bit_old)
		{
			apu.psg.advance_sequencer(); // advance the sequencer
		}
	}

	
	// falling edge for sound clk which allways ticks
	// done when timer is off 
	// (cycles should only ever be added once per function call)
	else 
	{
		internal_timer += cycles;
		if(is_set(internal_timer,sound_bit) != sound_bit_old)
		{
			apu.psg.advance_sequencer(); // advance the sequencer
		}
	}

	insert_new_timer_event();
}

void Cpu::insert_new_serial_event() noexcept
{
	const auto serial_event = scheduler.create_event(serial_cyc,gameboy_event::serial);
	scheduler.insert(serial_event,false); 
}


void Cpu::tick_serial(int cycles) noexcept
{
	if(is_set(mem.io[IO_SC],7))
	{
		serial_cyc -= cycles;
		if(serial_cyc <= 0)
		{
			// faster xfer set
			if(is_cgb && is_set(mem.io[IO_SC],1))
			{
				serial_cyc = 4;
			}

			else
			{
				serial_cyc = 128;
			}


			// "push" the data out
			// note we only emulate a disconected serial
			mem.io[IO_SB] <<= 1; // shift out data out
			mem.io[IO_SB] |= 1; // recive our data

			if(!--serial_cnt)
			{
				mem.io[IO_SC] = deset_bit(mem.io[IO_SC],7); // deset bit 7 to say tranfser has ended
				request_interrupt(3);				
			}

			else
			{
				insert_new_serial_event();
			}
		}
	}
}

void Cpu::handle_halt()
{
	// smash off all pending cycles before the halt check
	tick_pending_cycles(); 

	if(scheduler.size() == 0)
	{
		throw std::runtime_error("halt infinite loop");
	}


	instr_side_effect = instr_state::normal;

	// halt bug
	// halt state not entered and the pc fails to increment for
	// one instruction read 			
	if( !interrupt_enable && interrupt_req)
	{
		halt_bug = true;
		return;
	}
	

	// sanity check to check if this thing will actually fire
	// needs to be improved to check for specific intrs being unable to fire...
	if( mem.io[IO_IE] == 0)
	{
		write_log(debug,"[ERROR] halt infinite loop");
		throw std::runtime_error("halt infinite loop");
	}

	// if in mid scanline mode tick till done
	// else just service events

	// still need to check that intr are not firing during this
	while(ppu.using_fifo() &&  !interrupt_req)
	{
		cycle_tick(1);
	}

	while(!interrupt_req)
	{
		// if there are no events 
		// we are stuck
		if(scheduler.size() == 0)
		{
			throw std::runtime_error("halt infinite loop");
		}

		scheduler.skip_to_event();
	}


/*
	// old impl
	while(!interrupt_req)
	{
		cycle_tick(1);
	}
*/
}


// interrupts
// needs accuracy improvement with the precise interrupt 
// timings to pass ie_push

void Cpu::request_interrupt(int interrupt) noexcept
{
	// set the interrupt flag to signal
	// an interrupt request
	mem.io[IO_IF] = set_bit( mem.io[IO_IF],interrupt);
	update_intr_req();
}


void Cpu::do_interrupts() noexcept
{

	// interrupt has fired disable ime
	interrupt_enable = false;
	// ime is off we cant have a fire
	interrupt_fire = false;

	// sp deced in 3rd cycle not sure what happens in 2nd
	cycle_tick(2);

	// first sp push on 4th cycle
	write_stackt((pc & 0xff00) >> 8);

	// 5th cycle in middle of stack push ie and if are checked to  get the 
	// fired interrupt
	cycle_tick_t(2);
	const auto flags = mem.io[IO_IF] & mem.io[IO_IE];
	cycle_tick_t(2);

	oam_bug_write(pc);
	write_stack(pc & 0xff);

	// 6th cycle the opcode prefetch will happen
	// we handle this after this function

	// priority for servicing starts at interrupt 0
	// figure out what interrupt has fired
	// if any at this point
#ifdef _MSC_VER
	for(int i = 0; i < 5; i++)
	{
		// if requested & is enabled
		if(is_set(flags,i))
		{
			mem.io[IO_IF] = deset_bit(mem.io[IO_IF],i); // mark interrupt as serviced
			update_intr_req();
			pc = 0x40 + (i * 8); // set pc to interrupt vector
			return;
		}
	}
#else
	if(flags != 0)
	{
		const auto bit = __builtin_ctz(flags);
		mem.io[IO_IF] = deset_bit(mem.io[IO_IF],bit); // mark interrupt as serviced
		update_intr_req();
		pc = 0x40 + (bit * 8); // set pc to interrupt vector
		return;		
	}
#endif

	// interrupt did fire but now is no longer requested
	// pc gets set to zero
	pc = 0;
}





// util for opcodes

// register getters and setters
void Cpu::write_bc(uint16_t data) noexcept
{
    b = (data & 0xff00) >> 8;
    c = data & 0x00ff;
}


uint16_t Cpu::read_bc(void) const noexcept
{
    return (b << 8) | c;
}


uint16_t Cpu::read_af(void) const noexcept
{
    return (a << 8) | carry << C | half << H
		| zero << Z | negative << N;
}



void Cpu::write_af(uint16_t v) noexcept 
{
    a = (v & 0xff00) >> 8;
    carry = is_set(v,C);
	half = is_set(v,H);
	zero = is_set(v,Z);
	negative = is_set(v,N);
}

uint16_t Cpu::read_de(void) const noexcept 
{
    return (d << 8) | e;
}


void Cpu::write_de(uint16_t v) noexcept
{
    d = (v & 0xff00) >> 8;
    e = v & 0x00ff;
}


uint16_t Cpu::read_hl(void) const noexcept
{
    return (h << 8) | l;
}


void Cpu::write_hl(uint16_t v) noexcept
{
    h = (v & 0xff00) >> 8;
    l = v & 0x00ff;
}


void Cpu::write_stackt(uint8_t v) noexcept
{
	oam_bug_write(sp);
	// need to ignore oam triggers for this one
	mem.write_memt_no_oam_bug(--sp,v); // write to stack
}


void Cpu::write_stackwt(uint16_t v) noexcept
{	
	write_stackt((v & 0xff00) >> 8);
	write_stackt((v & 0x00ff));
}

// unticked only used by interrupts
void Cpu::write_stack(uint8_t v) noexcept
{
	mem.write_mem(--sp,v); // write to stack
}

void Cpu::write_stackw(uint16_t v) noexcept
{
	write_stack((v & 0xff00) >> 8);
	write_stack((v & 0x00ff));
}

uint8_t Cpu::read_stackt() noexcept
{	
	return mem.read_memt_no_oam_bug(sp++);
}

uint16_t Cpu::read_stackwt() noexcept
{
	oam_bug_read_increment(sp);
	const auto v1 = read_stackt();

	oam_bug_read(sp);
	const auto v2 = read_stackt(); 

	return v1 | v2 << 8;
}


// oam bug
// https://gbdev.io/pandocs/#sprite-ram-bug
// todo trigger on read and writes along with interrupts
// this is still buggy for effect tests
uint32_t Cpu::get_cur_oam_row() const
{
	// ok so get the row off the scheduler
	// oam is 20 by 8 rows
	// so we basically just need how many m cycles have passed on the ppu
	// we assume here that  this cant get called when the lcd is off
	// as the oam bug wont trigger in that mode anyways
	const auto cycles_opt = scheduler.get_event_ticks(gameboy_event::ppu);
	if(!cycles_opt || ppu.get_mode() != ppu_mode::oam_search)
	{
		printf("ppu event not active during oam_bug!?\n");
		exit(1);
	}
	const auto row = cycles_opt.value() / 4;
	//printf("%zd\n",row);
	if(row >= 20)
	{
		printf("oam row >= 20!?");
		exit(1);
	}
	return row; 
}

bool Cpu::oam_should_corrupt(uint16_t v) const noexcept
{
	// only affects dmg (i think the tests have to be forced to dmg...)
	if(is_cgb)
	{
		return false;
	}

	// not in oam search dont care no corruption will happen
	if(ppu.get_mode() != ppu_mode::oam_search)
	{
		return false;
	}

	// if not in oam range we dont care...
	// this includes the "unused" range
	if(!(v >= 0xfe00 && v <= 0xfeff))
	{
		return false;
	}

	return true;
}

// eqiv to an increment
void Cpu::oam_bug_write(uint16_t v)
{
	tick_pending_cycles();
	if(!oam_should_corrupt(v))
	{
		return;
	}
	

	const auto row = get_cur_oam_row();
	//printf("oam corruption: %d\n",row);
	// nothing happens on the first row
	if(row == 0)
	{
		return;
	}

	// first word overwritten with corruption
	const uint16_t addr = row*8;

	const auto addr_prev = (row-1)*8;
	const auto a = handle_read<uint16_t>(mem.oam,addr);
	const auto b = handle_read<uint16_t>(mem.oam,addr_prev);
	const auto c = handle_read<uint16_t>(mem.oam,addr_prev + 4);

	const uint16_t corruption = ((a ^ c) & (b ^ c)) ^ c;

	handle_write<uint16_t>(mem.oam,addr,corruption);

	// last 3 words overwritten with ones from previous row
	memcpy(&mem.oam[addr+2],&mem.oam[addr_prev + 2],6);
}

void Cpu::oam_bug_read(uint16_t v)
{
	tick_pending_cycles();
	if(!oam_should_corrupt(v))
	{
		return;
	}


	const auto row = get_cur_oam_row();
	//printf("oam corruption: %d\n",row);
	// nothing happens on the first row
	if(row == 0)
	{
		return;
	}

	// first word overwritten with corruption
	const uint16_t addr = row*8;

	const auto addr_prev = (row-1)*8;
	const auto a = handle_read<uint16_t>(mem.oam,addr);
	const auto b = handle_read<uint16_t>(mem.oam,addr_prev);
	const auto c = handle_read<uint16_t>(mem.oam,addr_prev + 4);

	const uint16_t corruption = b | (a & c);

	handle_write<uint16_t>(mem.oam,addr,corruption);

	// last 3 words overwritten with ones from previous row
	memcpy(&mem.oam[addr+2],&mem.oam[addr_prev + 2],6);

}

void Cpu::oam_bug_read_increment(uint16_t v)
{
	tick_pending_cycles();
	if(!oam_should_corrupt(v))
	{
		return;
	}

	const auto row = get_cur_oam_row();
	//printf("oam corruption: %d\n",row);
	// nothing happens on the first rows or last
	if(row < 4 || row == 19)
	{
		return;
	}

	// first word overwritten with corruption
	const uint16_t addr = row*8;

	const auto addr_prev = (row-1)*8;
	const auto a = handle_read<uint16_t>(mem.oam,(row-2)*8);
	const auto b = handle_read<uint16_t>(mem.oam,addr_prev);
	const auto c = handle_read<uint16_t>(mem.oam,addr);	
	const auto d = handle_read<uint16_t>(mem.oam,addr_prev+4);

	const uint16_t corruption = (b & (a | c | d)) | (a & c & d);
	handle_write<uint16_t>(mem.oam,addr_prev,corruption);
	memcpy(&mem.oam[addr+2],&mem.oam[addr_prev + 2],6);
	memcpy(&mem.oam[((row-2)*8)+2],&mem.oam[addr_prev + 2],6);

}


}