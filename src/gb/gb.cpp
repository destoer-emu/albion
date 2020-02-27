#include <gb/gb.h>
#include <destoer-emu/key.h>

namespace gameboy
{

GB::GB()
{
	// setup a dummy state
	reset("",false);
}

void GB::reset(std::string rom_name, bool with_rom)
{
    mem.init(&cpu,&ppu,&debug,&apu,rom_name,with_rom);
    cpu.init(&mem,&ppu,&apu,&disass,&debug);
    ppu.init(&cpu,&mem);
    disass.init(&mem);
	apu.init(&mem);
	throttle_emu = true;
}


void GB::key_input(int key, bool pressed)
{


	if(pressed)
	{
		switch(key)
		{
			case emu_key::a: key_pressed(4); break;
			case emu_key::s: key_pressed(5); break;
			case emu_key::enter: key_pressed(7); break;
			case emu_key::space: key_pressed(6); break;
			case emu_key::right: key_pressed(0); break;
			case emu_key::left: key_pressed(1); break;
			case emu_key::up: key_pressed(2);break;
			case emu_key::down: key_pressed(3); break;
		}
	}

	else // released
	{
		switch(key)
		{
			case emu_key::a: key_released(4); break;
			case emu_key::s: key_released(5); break;
			case emu_key::enter: key_released(7); break;
			case emu_key::space: key_released(6); break;
			case emu_key::right: key_released(0); break;
			case emu_key::left: key_released(1); break;
			case emu_key::up: key_released(2);break;
			case emu_key::down: key_released(3); break;

			case emu_key::plus:
			{
				apu.stop_audio();
				throttle_emu = false;
				break;
			}

			case emu_key::minus:
			{
				apu.start_audio();
				throttle_emu = true;						
				break;
			}
		}
	}
}

// our "frontend" will call these
void GB::key_released(int key)
{
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

}