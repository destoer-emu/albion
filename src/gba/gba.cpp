#include <gba/gba.h>
#include <albion/input.h>


namespace gameboyadvance
{

// init memory here are make init function
// allow a dummy init
GBA::GBA()
{
#ifdef DEBUG
	change_breakpoint_enable(false);
#endif
}

#ifdef DEBUG
void GBA::change_breakpoint_enable(bool enabled)
{
	cpu.change_breakpoint_enable(enabled);
	mem.change_breakpoint_enable(enabled);
}
#endif

// init all sub compenents
void GBA::reset(std::string filename)
{
	scheduler.init();
    mem.init(filename);
    //disass.init(); 
    disp.init();
	apu.init();
    cpu.init();
	write_log(debug,"[new gba instance] {}",filename);
	throttle_emu = true;
}


// run a frame
void GBA::run()
{
	disp.new_vblank = false;	
#ifdef DEBUG
	if(debug.is_halted())
	{
		return;
	}
#endif

	// break out early if we have hit a debug event
	while(!disp.new_vblank) 
    {
		while(!scheduler.event_ready() && !cpu.interrupt_ready())
		{
			cpu.exec_instr();
		#if DEBUG
			if(debug.is_halted())
			{
				return;
			}
		#endif
		}
		scheduler.service_events();
		cpu.do_interrupts();
	}

	if(throttle_emu)
	{
		mem.frame_end();
	}
}



void GBA::handle_input(Controller& controller)
{
	for(auto& event : controller.input_events)
	{
		const b32 pressed = event.down;

		switch(event.input)
		{
			case controller_input::start: button_event(button::start,pressed); break;		
			case controller_input::select: button_event(button::select,pressed); break;
			case controller_input::down: button_event(button::down,pressed); break;
			case controller_input::up: button_event(button::up,pressed); break;
			case controller_input::left: button_event(button::left,pressed); break;
			case controller_input::right: button_event(button::right,pressed); break;
			case controller_input::a: button_event(button::a,pressed); break;
			case controller_input::x: button_event(button::b,pressed); break;
			case controller_input::left_trigger: button_event(button::l,pressed); break;
			case controller_input::right_trigger: button_event(button::r,pressed); break;

			default: break;
		}
	}
}

// we will decide if we are going to switch our underlying memory
// for io after the test 
void GBA::button_event(button b, bool down)
{
	int idx = static_cast<int>(b);

	auto &keyinput = mem.mem_io.keyinput;

	// 0 = pressed

	if(down)
	{
		keyinput = deset_bit(keyinput,idx); 
	}

	else
	{
		keyinput = set_bit(keyinput,idx);
	}

	mem.check_joypad_intr();
}

}