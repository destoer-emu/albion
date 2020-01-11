#include "../headers/cpu.h"
#include "../headers/memory.h"
#include "../headers/ppu.h"
#include "../headers/apu.h"
#include "../headers/debug.h"

void Cpu::init(Memory *m, Ppu *p,Apu *ap, Disass *dis, Debug *debugger)
{

	write_log("[INFO] new instance started!");

    mem = m;
    ppu = p;
    disass = dis;
	debug = debugger;
	apu = ap;

	uint8_t test = mem->read_mem(0x143);

	switch(test)
	{
		case 0x80: is_cgb = true; break; // add options to run cgb in dmg1
		case 0xc0: is_cgb = true; break;
		default: is_cgb = false; break;
	}
	
	// set the cgb initial registers 
	if(is_cgb)
	{
		a = 0x11; f = 0x80; // af = 0x1180;
		b = 0x00; c = 0x00; // bc = 0x0000;
		d = 0xff; e = 0x56; // de = 0xff56;
		h = 0x00; l = 0x0d; // hl = 0x000d;
		sp = 0xfffe;
		pc = 0x100;
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

	is_double = false;
    internal_timer = 0;
    
    joypad_state = 0xff;

    // interrupts
    halt = false;
    ei = false;
   	di = false;
    interrupt_enable = false;
    halt_bug = false;

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
	// should operate at double speed
	int factor = is_double ? 2 : 1;

	// timers act at constant speed
	update_timers(cycles*4); 

	// handler will check if its enabled
	mem->tick_dma(cycles*factor);
	
	// in double speed mode gfx and apu should operate at half
	ppu->update_graphics((cycles*4) / factor); // handle the lcd emulation
	apu->tick((cycles*4) / factor); // advance the apu state
}



void Cpu::update_timers(int cycles)
{
	
	int sound_bit = is_double? 13 : 12;

	bool sound_bit_set = is_set(internal_timer,sound_bit);
	

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
        
		// we repeat this here because we have to add the cycles
		// at the proper time for the falling edge det to work
		// and we dont want to waste time handling the falling edge
		// for the timer when its off
		if(!is_set(internal_timer,sound_bit) && sound_bit_set)
		{
			apu->advance_sequencer(); // advance the sequencer
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
			apu->advance_sequencer(); // advance the sequencer
		}
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
		
		// normal halt
				
		else 
		{
			// sanity check to check if this thing will actually fire

			uint8_t stat = mem->io[IO_STAT];
			if(enabled == 0 || ((((stat >> 3) & 0x7) == 0) && enabled == val_bit(enabled,1)))
			{
				write_log("[ERROR] halt infinite loop");
				throw std::runtime_error("halt infinite loop");
			}



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
		case 1: pc = 0x48; break; //lcd-stat 
		case 2: pc = 0x50; break; // timer 
		case 3: pc = 0x58; break; //serial (not fully implemented)
		case 4: pc = 0x60; break; // joypad
		default: printf("Invalid interrupt %d at %x\n",interrupt,pc); exit(1);
	}	
}



// util for opcodes

// register getters and setters
void Cpu::write_bc(uint16_t data) 
{
    b = (data & 0xff00) >> 8;
    c = data & 0x00ff;
}


uint16_t Cpu::read_bc(void) const 
{
    return (b << 8) | c;
}


uint16_t Cpu::read_af(void) const
{
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