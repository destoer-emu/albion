#include<gb/gb.h>

namespace gameboy
{

Cpu::Cpu(GB &gb) : mem(gb.mem), apu(gb.apu), ppu(gb.ppu), 
	debug(gb.debug), disass(gb.disass)
{
	
}

void Cpu::init(bool use_bios)
{
	write_log(debug,"[INFO] new instance started!");


	is_cgb = mem.rom_cgb_enabled();

	// setup regs to skip the bios
	if(!use_bios)
	{
		// set the cgb initial registers 
		if(is_cgb)
		{
			a = 0x11; f = 0x80; // af = 0x1180;
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
			a = 0x01; f = 0xb0; // af = 0x01b0
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
		a = 0x00; f = 0x00; // af = 0x0000
		b = 0x00; c = 0x00; // bc = 0x0000
		d = 0x00; e = 0x00; // de = 0x0000
		h = 0x00; l = 0x00; // hl = 0x0000
		sp = 0x0000;
		pc = 0x0000;		
	}


	is_double = false;
    internal_timer = 0;
    
    joypad_state = 0xff;

    // interrupts
	instr_side_effect = instr_state::normal;
    interrupt_enable = false;
    halt_bug = false;

}


void Cpu::step()
{
	// interrupts checked before the opcode fetch
    exec_instr();


	// now we need to test if an ei or di instruction
	// has just occured if it has step a cpu instr and then 
	// perform the requested operation and set the ime flag	
	handle_instr_effects();
}


uint8_t Cpu::fetch_opcode() noexcept
{
	// at midpoint of instr fetch interrupts are checked
	// and if so the opcode is thrown away and interrupt dispatch started
	cycle_tick_t(2);
	bool fired = ((mem.io[IO_IF] & mem.io[IO_IE] & 0x1f) && interrupt_enable);
	cycle_tick_t(2);

	if(fired)
	{
		do_interrupts();

		// have to re fetch the opcode this costs a cycle
		return mem.read_memt(pc++);
	}

	else // return the opcode we have just fetched
	{
		return mem.read_mem(pc++);
	}
	
}

// m cycle tick
void Cpu::cycle_tick(int cycles) noexcept
{
	//  convert to t cycles and tick
	cycle_tick_t(cycles * 4);
}

// t cycle tick
void Cpu::cycle_tick_t(int cycles) noexcept
{

	// timers act at constant speed
	update_timers(cycles); 

	// handler will check if its enabled
	mem.tick_dma(cycles << is_double);
	
	// in double speed mode gfx and apu should operate at half
	ppu.update_graphics(cycles >> is_double); // handle the lcd emulation
	apu.tick(cycles >> is_double); // advance the apu state	
}

void Cpu::tima_inc() noexcept
{
	// timer about to overflow
	if(mem.io[IO_TIMA] == 255)
	{	
		mem.io[IO_TIMA] = mem.io[IO_TMA]; // reset to value in tma
		request_interrupt(2); // timer overflow interrupt			
	}
			
	else
	{
		mem.io[IO_TIMA]++;
	}	
}

bool Cpu::internal_tima_bit_set() const noexcept
{
    // freq bits for internal timer
    static constexpr int freq_arr[4] = {9,3,5,7};

	const uint8_t freq = mem.io[IO_TMC] & 0x3;

	const int bit = freq_arr[freq];

	return is_set(internal_timer,bit);
}

bool Cpu::tima_enabled() const noexcept
{
	return is_set(mem.io[IO_TMC],2);	
}

void Cpu::update_timers(int cycles) noexcept
{
	
	const int sound_bit = is_double? 13 : 12;

	const bool sound_bit_set = is_set(internal_timer,sound_bit);
	

	// if our bit is deset and it was before (falling edge)
	// and the timer is enabled of course
	if(tima_enabled())
	{
		bool bit_set = internal_tima_bit_set();

		internal_timer += cycles;

		if(!internal_tima_bit_set() && bit_set)
		{
			tima_inc();
		}
        
		// we repeat this here because we have to add the cycles
		// at the proper time for the falling edge det to work
		// and we dont want to waste time handling the falling edge
		// for the timer when its off
		if(!is_set(internal_timer,sound_bit) && sound_bit_set)
		{
			apu.advance_sequencer(); // advance the sequencer
		}
        
	}

    
	// falling edge for sound clk which allways ticks
	// done when timer is off 
	// (cycles should only ever be added once per function call)
	else 
	{
		internal_timer += cycles;
		if(!is_set(internal_timer,sound_bit) && sound_bit_set)
		{
			apu.advance_sequencer(); // advance the sequencer
		}
	} 
       
}


void Cpu::handle_halt()
{
	instr_side_effect = instr_state::normal;
	uint8_t req = mem.io[IO_IF]; // requested ints 
	uint8_t enabled = mem.io[IO_IE]; // enabled interrutps
	
	// halt bug
	// halt state not entered and the pc fails to increment for
	// one instruction read 			
	if( (interrupt_enable == false) &&  (req & enabled & 0x1f) != 0)
	{
		halt_bug = true;
		return;
	}
	

	// sanity check to check if this thing will actually fire
	const uint8_t stat = mem.io[IO_STAT];
	if(enabled == 0 || ((((stat >> 3) & 0x7) == 0) && enabled == val_bit(enabled,1)))
	{
		write_log(debug,"[ERROR] halt infinite loop");
		throw std::runtime_error("halt infinite loop");
	}

	
	while( ( req & enabled & 0x1f) == 0) 
	{
		// just tick it
		cycle_tick(1);
			
		req = mem.io[IO_IF];
	}


	/*
	// ideally we should just figure out how many cycles to the next interrupt

	// halt is immediatly over we are done
	if(req & enabled & 0x1f)
	{
		return;
	}

	int cycles_to_event;

	// check if timer interrupt enabled (and the timer is enabled) if it is
	// determine when it will fire
	if(is_set(mem.io[IO_TMC],2) && is_set(enabled,2))
	{

	}

	// determine when next stat inerrupt will fire
	// because of the irq stat glitches if its on we have to figure out when it first goes off
	// and then re run the check additonally if our target ends in hblank we need to step it manually
	// as pixel transfer -> hblank takes a variable number of cycles
	// (allthough try to come up with a method to actually calculate it based on number of sprites scx etc)


	// whichever interrupt hits first tick until it :)
	*/

}

// handle the side affects of instructions
void Cpu::handle_instr_effects()
{
	switch(instr_side_effect)
	{
		// no instr side effects
		case instr_state::normal:
		{
			break;
		}

		case instr_state::ei: // ei
		{
			instr_side_effect = instr_state::normal;
			exec_instr(); 
			// we have done an instruction now set ime
			// needs to be just after the instruction service
			// but before we service interrupts
			
			if(instr_side_effect != instr_state::di) // if we have just executed di do not renable interrupts
			{	
				interrupt_enable = true;
			}
					
			if(instr_side_effect == instr_state::halt)
			{
				handle_halt();
			}

			break;
		}
				
		case instr_state::di:  // di
		{
			instr_side_effect = instr_state::normal;
			interrupt_enable = false; // di should disable immediately unlike ei!
			break;
		}
			
		// this will make the cpu stop executing instr
		// until an interrupt occurs and wakes it up 			
		case instr_state::halt: // halt occured in prev instruction
		{		
			handle_halt();
			break;
		}
	}
}


// interrupts
// needs accuracy improvement with the precise interrupt 
// timings to pass ie_push

void Cpu::request_interrupt(int interrupt) noexcept
{
	// set the interrupt flag to signal
	// an interrupt request
	mem.io[IO_IF] = set_bit( mem.io[IO_IF],interrupt);
}


void Cpu::do_interrupts() noexcept
{

	// interrupt has fired disable ime
	interrupt_enable = false;

	// sp deced in 3rd cycle not sure what happens in 2nd
	cycle_tick(2);

	// first sp push on 4th cycle
	write_stackt((pc & 0xff00) >> 8);

	// 5th cycle in middle of stack push ie and if are checked to  get the 
	// fired interrupt
	cycle_tick_t(2);
	uint8_t req = mem.io[IO_IF];
	uint8_t enabled = mem.io[IO_IE];
	cycle_tick_t(2);

	write_stack(pc & 0xff);

	// 6th cycle the opcode prefetch will happen
	// we handle this after this function

	// priority for servicing starts at interrupt 0
	// figure out what interrupt has fired
	// if any at this point
	for(int i = 0; i < 5; i++)
	{
		// if requested & is enabled
		if(is_set(req,i) && is_set(enabled,i))
		{
			mem.io[IO_IF] = deset_bit(mem.io[IO_IF],i); // mark interrupt as serviced
			pc = 0x40 + (i * 8); // set pc to interrupt vector
			return;
		}
	}

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
    return (a << 8) | f;
}



void Cpu::write_af(uint16_t v) noexcept 
{
    a = (v & 0xff00) >> 8;
    f = v & 0x00f0; // only top four bits of flag is active
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
	mem.write_memt(--sp,v); // write to stack
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
	return mem.read_memt(sp++);
}

uint16_t Cpu::read_stackwt() noexcept
{
	return read_stackt() | (read_stackt() << 8);
}

}