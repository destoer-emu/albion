#include <gb/gb.h>
#include <destoer-emu/key.h>

namespace gameboy
{

GB::GB()
{
	// setup a dummy state
	reset("no rom",false,false);
}

void GB::reset(std::string rom_name, bool with_rom, bool use_bios)
{
    mem.init(rom_name,with_rom,use_bios);
    ppu.init();
    disass.init();
	apu.init();
	cpu.init(use_bios);
	throttle_emu = true;
	if(use_bios)
	{
		mem.bios_enable();
	}
}


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

			case emu_key::plus:
			{
				apu.playback.stop();
				throttle_emu = false;
				break;
			}

			case emu_key::minus:
			{
				apu.playback.start();
				throttle_emu = true;						
				break;
			}

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
try
{
	std::ofstream fp(filename,std::ios::binary);

	cpu.save_state(fp);
	mem.save_state(fp);
	ppu.save_state(fp);
	apu.save_state(fp);

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
try
{	
	std::ifstream fp(filename,std::ios::binary);

	cpu.load_state(fp);
	mem.load_state(fp);
	ppu.load_state(fp);
	apu.load_state(fp);

	fp.close();
}


catch(std::exception &ex)
{
	std::string err = fmt::format("failed to load state: {}",ex.what());
	debug.write_logger(err);
	throw std::runtime_error(err);
}

}

void GB::key_pressed(button b)
{
	auto key = static_cast<int>(b);

	bool previously_unset = false;
	
	// if setting from 1 to 0 we may have to req 
	// and interrupt
	if(!is_set(cpu.joypad_state,key))
	{
		previously_unset = true;
	}
	
	// remember if a key is pressed its bit is 0 not 1
	cpu.joypad_state = deset_bit(cpu.joypad_state, key);
	

	// is the input for a button?
	bool button = (key > 3);

	
	uint8_t key_req = mem.io[IO_JOYPAD];
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
	
	// fire an interrupt
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

}