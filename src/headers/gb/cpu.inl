#pragma once
#include <gb/gb.h>

namespace gameboy
{

// m cycle tick
inline void Cpu::cycle_tick(uint32_t cycles) noexcept
{
	//  convert to t cycles and tick
	cycle_tick_t(cycles * 4);
}


// t cycle tick
inline void Cpu::cycle_tick_t(uint32_t cycles) noexcept
{
/*
	// timers act at constant speed
	update_timers(cycles); 

	// handler will check if its enabled
	mem.tick_dma(cycles);
	
	// in double speed mode gfx and apu should operate at half
	ppu.update_graphics(cycles >> is_double); // handle the lcd emulation

	apu.tick(cycles >> is_double); // advance the apu state	

	tick_serial(cycles);
*/
	scheduler.delay_tick(cycles);

	// if we are using the fifo this needs to be ticked each time
	if(ppu.emulate_pixel_fifo)
	{
		scheduler.service_events();
		ppu.update_graphics(cycles >> is_double); // handle the lcd emulation
	}

}


inline void Cpu::write_stackt(uint8_t v) noexcept
{
	oam_bug_write(sp);
	// need to ignore oam triggers for this one
	mem.write_memt_no_oam_bug(--sp,v); // write to stack
}


inline void Cpu::write_stackwt(uint16_t v) noexcept
{	
	write_stackt((v & 0xff00) >> 8);
	write_stackt((v & 0x00ff));
}

// unticked only used by interrupts
inline void Cpu::write_stack(uint8_t v) noexcept
{
	mem.write_mem(--sp,v); // write to stack
}

inline void Cpu::write_stackw(uint16_t v) noexcept
{
	write_stack((v & 0xff00) >> 8);
	write_stack((v & 0x00ff));
}

inline uint8_t Cpu::read_stackt() noexcept
{	
	return mem.read_memt_no_oam_bug(sp++);
}

inline uint16_t Cpu::read_stackwt() noexcept
{
	oam_bug_read_increment(sp);
	const auto v1 = read_stackt();

	oam_bug_read(sp);
	const auto v2 = read_stackt(); 

	return v1 | v2 << 8;
}


}