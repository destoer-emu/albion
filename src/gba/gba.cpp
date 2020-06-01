#include <gba/gba.h>
#include <destoer-emu/key.h>


namespace gameboyadvance
{

// init all sub compenents
void GBA::reset(std::string filename)
{
    mem.init(filename);
    //disass.init(); 
    disp.init();
	apu.init();
    cpu.init();
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



void GBA::key_input(int key, bool pressed)
{
	switch(static_cast<emu_key>(key))
	{
		case emu_key::enter: button_event(button::start,pressed); break;		
		case emu_key::space: button_event(button::select,pressed); break;
		case emu_key::down: button_event(button::down,pressed); break;
		case emu_key::up: button_event(button::up,pressed); break;
		case emu_key::left: button_event(button::left,pressed); break;
		case emu_key::right: button_event(button::right,pressed); break;
		case emu_key::a: button_event(button::a,pressed); break;
		case emu_key::s: button_event(button::b,pressed); break;
        case emu_key::d: button_event(button::l,pressed); break;
        case emu_key::f: button_event(button::r,pressed); break;
		default: break;
	}	
}

// we will decide if we are going to switch our underlying memory
// for io after the test 
void GBA::button_event(button b, bool down)
{
	int idx = static_cast<int>(b);

	auto &keyinput = mem.mem_io.keyinput;
	auto &key_control = mem.mem_io.key_control;

	// 0 = pressed

	if(down)
	{
		keyinput = deset_bit(keyinput,idx); 
	}

	else
	{
		keyinput = set_bit(keyinput,idx);
	}

	// keyinput irqs enabled
	if(key_control.irq_enable_flag)
	{
		int res = key_control.key_cnt & keyinput & 0x3FF; 

		// one pressed
		if(key_control.irq_cond)
		{
			// if any key is pressed we care about fire
			if(res > 0)
			{
				cpu.request_interrupt(interrupt::keypad);
			}
		}

		// all pressed
		else if(res == (key_control.key_cnt & 0x3ff))
		{
			cpu.request_interrupt(interrupt::keypad);
		}
	}	
}

}