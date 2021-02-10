#include <gb/gb.h>
#include <destoer-emu/key.h>

namespace gameboy
{

GB::GB()
{
	// setup a dummy state
	reset("no rom",false,false);
	// default the breakpoints to off
	// maybe something to consider as a config
#ifdef DEBUG
	change_breakpoint_enable(false);
#endif
}

void GB::reset(std::string rom_name, bool with_rom, bool use_bios)
{
	//use_bios = true;
	scheduler.init();
    mem.init(rom_name,with_rom,use_bios);
	cpu.init(use_bios);
    ppu.init();
    disass.init();
	apu.init(cpu.get_cgb(),use_bios);
	throttle_emu = true;
	if(use_bios)
	{
		mem.bios_enable();
	}
	//printf("cgb: %s\n",cpu.get_cgb()? "true" : "false");
}

#ifdef DEBUG
void GB::change_breakpoint_enable(bool enabled)
{
	debug.breakpoints_enabled = enabled;
	mem.change_breakpoint_enable(enabled);
	cpu.change_breakpoint_enable(enabled);
}
#endif

void GB::key_input(int key, bool pressed)
{


	if(pressed)
	{
		switch(static_cast<emu_key>(key))
		{
			case emu_key::a: key_pressed(button::a); break;
			case emu_key::s: key_pressed(button::b); break;
			case emu_key::enter: key_pressed(button::start); break;
			case emu_key::space: key_pressed(button::select); break;
			case emu_key::right: key_pressed(button::right); break;
			case emu_key::left: key_pressed(button::left); break;
			case emu_key::up: key_pressed(button::up); break;
			case emu_key::down: key_pressed(button::down); break;
			case emu_key::k:
			{
				apu.playback.stop();
				throttle_emu = false;
				#ifdef FRONTEND_IMGUI
					glfwSwapInterval(0); // Disable vsync
				#endif
				break;
			}

			case emu_key::l:
			{
				apu.playback.start();
				throttle_emu = true;
				#ifdef FRONTEND_IMGUI
					glfwSwapInterval(1); // Enable vsync
				#endif						
				break;
			}

			default: break;
		}
	}

	else // released
	{
		switch(static_cast<emu_key>(key))
		{
			case emu_key::a: key_released(button::a); break;
			case emu_key::s: key_released(button::b); break;
			case emu_key::enter: key_released(button::start); break;
			case emu_key::space: key_released(button::select); break;
			case emu_key::right: key_released(button::right); break;
			case emu_key::left: key_released(button::left); break;
			case emu_key::up: key_released(button::up); break;
			case emu_key::down: key_released(button::down); break;
			default: break;
		}
	}
}

// our "frontend" will call these
void GB::key_released(button b)
{
	auto key = static_cast<int>(b); 
	cpu.joypad_state = set_bit(cpu.joypad_state, key);
}


// need to do alot more integrity checking on data in these :)
void GB::save_state(std::string filename)
{

	std::cout << "save state: " << filename << "\n";
try
{
	std::ofstream fp(filename,std::ios::binary);
	if(!fp)
	{
		throw std::runtime_error("could not open file");
	}


	cpu.save_state(fp);
	mem.save_state(fp);
	ppu.save_state(fp);
	apu.save_state(fp);
	scheduler.save_state(fp);

	fp.close();
}

catch(std::exception &ex)
{
	std::string err = fmt::format("failed to save state: {}",ex.what());
	debug.write_logger(err);
	throw std::runtime_error(err);
}
}


void GB::load_state(std::string filename)
{
	std::cout << "load state: " << filename << "\n";

try
{	
	std::ifstream fp(filename,std::ios::binary);
	if(!fp)
	{
		throw std::runtime_error("could not open file");
	}

	cpu.load_state(fp);
	mem.load_state(fp);
	ppu.load_state(fp);
	apu.load_state(fp);
	scheduler.load_state(fp);

	fp.close();
}


catch(std::exception &ex)
{
	// put system back into a safe state
	reset("",false,false);
	std::string err = fmt::format("failed to load state: {}",ex.what());
	debug.write_logger(err);
	throw std::runtime_error(err);
}

}

void GB::key_pressed(button b)
{
	const auto key = static_cast<int>(b);

	// if setting from 1 to 0 we may have to req 
	// and interrupt
	const bool previously_unset = is_set(cpu.joypad_state,key);
	
	// remember if a key is pressed its bit is 0 not 1
	cpu.joypad_state = deset_bit(cpu.joypad_state, key);
	
	// is the input for a button?
	const bool button = (key > 3);

	
	const auto key_req = mem.io[IO_JOYPAD];
	bool req_int = false;
	
	// only fire interrupts on buttons the game is current trying to read
	// a, b, start, sel
	if(button && !is_set(key_req,5))
	{
		req_int = true;
	}
	
	// dpad
	else if(!button && !is_set(key_req,4))
	{
		req_int = true;
	}
	
	if(req_int && previously_unset)
	{
		cpu.request_interrupt(4);
	}
}

// run a frame
void GB::run()
{
    ppu.new_vblank = false;
#ifdef DEBUG
	// break out early if we have hit a debug event
	while(!ppu.new_vblank && !debug.is_halted()) 
    {
        cpu.exec_instr();
	}
#else 
	while(!ppu.new_vblank) // exec until a vblank hits
    {
        cpu.exec_instr();
	}
#endif
	mem.frame_end();
}

}