#include <gba/gba.h>


// init all sup compenents
void GBA::reset(std::string filename)
{
    mem.init(filename,&debug,&cpu,&disp);
    disass.init(&mem,&cpu);
    disp.init(&mem,&cpu);
    cpu.init(&disp,&mem,&debug,&disass);
	debug.write_logger("[new gba instance] {}",filename);
}


// run a frame
void GBA::run()
{

	while(!disp.new_vblank) // exec until a vblank hits
    {
        cpu.step();
	}

    disp.new_vblank = false;	
}

// we will decide if we are going to switch our underlying memory
// for io after the test 
void GBA::button_event(button b, bool down)
{
	uint16_t keyinput = mem.handle_read<uint16_t>(mem.io,IO_KEYINPUT&IO_MASK);

	int idx = static_cast<int>(b);

	// 0 = pressed

	if(down)
	{
		keyinput = deset_bit(keyinput,idx); 
	}

	else
	{
		keyinput = set_bit(keyinput,idx);
	}

	mem.handle_write<uint16_t>(mem.io,IO_KEYINPUT&IO_MASK,keyinput);
}