#include "headers/gb.h"


void GB::reset(std::string rom_name)
{
    mem.init(&cpu,&ppu,rom_name);
    cpu.init(&mem,&ppu,&disass);
    ppu.init(&cpu,&mem);
    disass.init(&mem);
}

// our "frontend" will call these
void GB::key_released(int key)
{
	cpu.joypad_state = set_bit(cpu.joypad_state, key);
}

void GB::key_pressed(int key)
{
	bool previously_unset = false;
	
	// if setting from 1 to 0 we may have to req 
	// and interrupt
	if(!is_set(cpu.joypad_state,key))
	{
		previously_unset = true;
	}
	
	// remember if a key is pressed its bit is 0 not 1
	cpu.joypad_state = deset_bit(cpu.joypad_state, key);
	

	
	
	// is this a standard button or a directional one?
	bool button = (key > 3);

	
	uint8_t key_req = mem.io[IO_JOYPAD];
	bool req_int = false;
	
	// only request an interrupt if the button just pressed
	// is the style of button the game is intereted in
	if(button && !is_set(key_req,5))
	{
		req_int = true;
	}
	
	// same but for direcitonals
	
	else if(!button && !is_set(key_req,4))
	{
		req_int = true;
	}
	
	// req an int
	if(req_int && !previously_unset)
	{
		cpu.request_interrupt(4);
	}
}

// run a frame
void GB::run()
{
    ppu.new_vblank = false;
    while(!ppu.new_vblank)
	{
        cpu.step();
	}
}