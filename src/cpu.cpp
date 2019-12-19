#include "headers/cpu.h"
#include "headers/memory.h"
#include "headers/ppu.h"

void Cpu::init(Memory *m, Ppu *p, Disass *d)
{
    mem = m;
    ppu = p;
    disass = d;


    // set our initial register state
    a = 0x01; f = 0xb0; // af = 0x01b0
    b = 0x00; c = 0x13; // bc = 0x0013
    d = 0x00; e = 0xd8; // de = 0x00d8
    h = 0x01; l = 0x4d; // hl = 0x014d
    sp = 0xfffe;
    pc = 0x0100;
}


void Cpu::step()
{
    exec_instr();

	// handle interrupts
	do_interrupts();

	// now we need to test if an ei or di instruction
	// has just occured if it has step a cpu instr and then 
	// perform the requested operation and set the ime flag	
	handle_instr_effects();
}



// just a stub for now
void Cpu::cycle_tick(int cycles)
{
	// function will check if its enabled
	mem->tick_dma(cycles);

	// timers act at constant speed
	update_timers(cycles*4); 
	ppu->update_graphics(cycles*4);
}



void Cpu::update_timers(int cycles)
{
	
	//int sound_bit = cpu->is_double? 13 : 12;

	//bool sound_bit_set = is_set(internal_timer,sound_bit);
	

	// if our bit is deset and it was before (falling edge)
	// and the timer is enabled of course
	if(is_set(mem->io[IO_TMC],2))
	{
		uint8_t freq = mem->io[IO_TMC] & 0x3;

		static constexpr int freq_arr[4] = {9,3,5,7};

		int bit = freq_arr[freq];

		bool bit_set = is_set(internal_timer,bit);

		internal_timer += cycles;

		if(!is_set(internal_timer,bit) && bit_set)
		{

			// timer about to overflow
			if(mem->io[IO_TIMA] == 255)
			{	
				mem->io[IO_TIMA] = mem->io[IO_TMA]; // reset to value in tma
				request_interrupt(2); // timer overflow interrupt			
			}
					
			else
			{
			    mem->io[IO_TIMA]++;
			}
		}
        /*
		// we repeat this here because we have to add the cycles
		// at the proper time for the falling edge det to work
		// and we dont want to waste time handling the falling edge
		// for the timer when its off
		if(!is_set(internal_timer,sound_bit) && sound_bit_set)
		{
			advance_sequencer(); // advance the sequencer
		}
        */
	}

    
	// falling edge for sound clk which allways ticks
	// done when timer is off 
	// (cycles should only ever be added once per function call)
	else 
	{
		internal_timer += cycles;
	/*	if(!is_set(cpu->internal_timer,sound_bit) && sound_bit_set)
		{
			advance_sequencer(cpu); // advance the sequencer
		}
	*/
	} 
       
}

// handle the side affects of instructions
void Cpu::handle_instr_effects()
{
	if(ei) // ei
	{
		ei = false; // assume a di was executed next instr
		exec_instr(); 
		// we have done an instruction now set ime
		// needs to be just after the instruction service
		// but before we service interrupts
		
		if(!di) // if we have just executed di do not renable interrupts
		{	
			interrupt_enable = true;
		}
				
		do_interrupts(); // handle interrupts 
	}
			
	else if(di) // di
	{
		di = false;
		interrupt_enable = false; // di should disable immediately unlike ei!
	}
		
	// this will make the cpu stop executing instr
	// until an interrupt occurs and wakes it up 			
	else if(halt) // halt occured in prev instruction
	{		
		halt = false;

		uint8_t req = mem->io[IO_IF]; // requested ints 
		uint8_t enabled = mem->io[IO_IE]; // enabled interrutps
		
		// halt bug
		// halt state not entered and the pc fails to increment for
		// one instruction read 			
		if( (interrupt_enable == false) &&  (req & enabled & 0x1f) != 0)
		{
			halt_bug = true;
		}

				
		/*// not sure what defined behaviour is here
		else if(enabled == 0)
		{
	
		}*/
				
		// normal halt
				
		else 
		{
			while( ( req & enabled & 0x1f) == 0) 
			{
				// just tick it
				cycle_tick(1);
					
				req = mem->io[IO_IF];
				enabled = mem->io[IO_IE];
			}
			do_interrupts(); // handle interrupts
		}
	}
}



// interrupts
// needs accuracy improvement with the precise interrupt 
// timings to pass ie_push

void Cpu::request_interrupt(int interrupt)
{
	// set the interrupt flag to signal
	// an interrupt request
	uint8_t req = mem->io[IO_IF];
	req = set_bit(req,interrupt);
	mem->io[IO_IF] = req;
}



void Cpu::do_interrupts()
{
	// if interrupts are enabled
	if(interrupt_enable)
	{	
		// get the set requested interrupts
		uint8_t req = mem->io[IO_IF];
		// checked that the interrupt is enabled from the ie reg 
		uint8_t enabled = mem->io[IO_IE];
		
		if(req > 0)
		{
			// priority for servicing starts at interrupt 0
			for(int i = 0; i < 5; i++)
			{
				// if requested & is enabled
				if(is_set(req,i) && is_set(enabled,i))
				{
					service_interrupt(i);
					cycle_tick(5); // every interrupt service costs 5 M cycles <-- break this up tomorrow
					return;
				}
			}
		}
	}
}

void Cpu::service_interrupt(int interrupt)
{
	interrupt_enable = false; // disable interrupts now one is serviced
		
	// reset the bit of in the if to indicate it has been serviced
	uint8_t req = mem->io[IO_IF];
	req = deset_bit(req,interrupt);
	mem->io[IO_IF] = req;
		
	// push the current pc on the stack to save it
	// it will be pulled off by reti or ret later
	write_stackw(pc);

		
	// set the program counter to the start of the
	// interrupt handler for the request interrupt
		
	switch(interrupt)
	{
		// interrupts are one less than listed in cpu manual
		// as our bit macros work from bits 0-7 not 1-8
		case 0: pc = 0x40; break; //vblank
		case 1: pc = 0x48; break; //lcd-state 
		case 2: pc = 0x50; break; // timer 
		case 3: pc = 0x58; break; //serial (not fully implemented)
		case 4: pc = 0x60; break; // joypad
		default: printf("Invalid interrupt %d at %x\n",interrupt,pc); exit(1);
	}	
}



// util for opcodes

// register getters and setters
void Cpu::write_bc(uint16_t data) {
    b = (data & 0xff00) >> 8;
    c = data & 0x00ff;
}


uint16_t Cpu::read_bc(void) const {
    return (b << 8) | c;
}


uint16_t Cpu::read_af(void) const {
    return (a << 8) | f;
}



void Cpu::write_af(uint16_t v) 
{
    a = (v & 0xff00) >> 8;
    f = v & 0x00f0; // only top four bits of flag is active
}

uint16_t Cpu::read_de(void) const 
{
    return (d << 8) | e;
}


void Cpu::write_de(uint16_t v) 
{
    d = (v & 0xff00) >> 8;
    e = v & 0x00ff;
}


uint16_t Cpu::read_hl(void) const 
{
    return (h << 8) | l;
}


void Cpu::write_hl(uint16_t v) 
{
    h = (v & 0xff00) >> 8;
    l = v & 0x00ff;
}


void Cpu::write_stackt(uint8_t v) 
{
	mem->write_memt(--sp,v); // write to stack
}

void Cpu::write_stackwt(uint16_t v) 
{
	write_stackt((v & 0xff00) >> 8);
	write_stackt((v & 0x00ff));
}

// unticked only used by interrupts
void Cpu::write_stack(uint8_t v) 
{
	mem->write_mem(--sp,v); // write to stack
}

void Cpu::write_stackw(uint16_t v) 
{
	write_stack((v & 0xff00) >> 8);
	write_stack((v & 0x00ff));
}

uint8_t Cpu::read_stackt() 
{	
	uint8_t data = mem->read_memt(sp++);
	return data;
}

uint16_t Cpu::read_stackwt() 
{
	return read_stackt() | (read_stackt() << 8);
}