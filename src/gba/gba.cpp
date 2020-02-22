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
	int idx = static_cast<int>(b);

	// 0 = pressed

	if(down)
	{
		mem.mem_io.keyinput = deset_bit(mem.mem_io.keyinput,idx); 
	}

	else
	{
		mem.mem_io.keyinput = set_bit(mem.mem_io.keyinput,idx);
	}	
}