#include <gb/gb.h>




// need to impl lcd enable timing side effects

namespace gameboy
{

Ppu::Ppu(GB &gb) : cpu(gb.cpu), mem(gb.mem),scheduler(gb.scheduler) 
{
	screen.resize(SCREEN_WIDTH*SCREEN_HEIGHT);
	std::fill(screen.begin(),screen.end(),0);	
}

void Ppu::reset_fetcher() noexcept
{
	x_cord = 0; // current x cord of the ppu
	tile_cord = 0;
	scx_cnt = 0;

	obj_fifo.reset();
	bg_fifo.reset();
	fetcher.reset();
}


// other stuff should proabably go in here i forgot to port over
void Ppu::init() noexcept
{
	std::fill(screen.begin(),screen.end(),0);

    // main ppu state
	mode = ppu_mode::oam_search;
	signal = false;
    scanline_counter = 0;
    current_line = 0;
    new_vblank = false;
	early_line_zero = false;

	emulate_pixel_fifo = false;

	reset_fetcher();

    // cgb pal
	sp_pal_idx = 0;
	bg_pal_idx = 0; // index into the bg pal (entry takes two bytes)

	window_y_line = 0;
	window_x_line = 0;

	window_x_triggered = false; 
	window_y_triggered = false;

	glitched_oam_mode = false;

	memset(bg_pal,0x00,sizeof(bg_pal)); // bg palette data
	memset(sp_pal,0x00,sizeof(sp_pal)); // sprite pallete data 

	// check if game can use in built pal in cgb rom
	if(!mem.rom_cgb_enabled())
	{
	/*
		// calc header checksum from 134 to 143 for name
		uint8_t checksum = 0;
		for(uint16_t i = 0x134; i < 0x143; i++)
		{
			checksum += mem.read_mem(i);
		}

		printf("checumsum %x\n",checksum);
	*/
		memcpy(dmg_pal,dmg_colors,sizeof(dmg_pal));

	} 

	insert_new_ppu_event();
}

// used for queing next ppu event
// callee will check if ppu is using pixel rendering
// or is off

// ppu scheduler causes bugs under 
// prehistorik man 
int Ppu::get_next_ppu_event() const noexcept
{

	switch(mode)
	{
		case ppu_mode::oam_search:
		{
			return OAM_END - scanline_counter;
		}

		case ppu_mode::pixel_transfer:
		{
			return pixel_transfer_end - scanline_counter;
		}

		case ppu_mode::hblank:
		{
			return LINE_END - scanline_counter;
		}

		case ppu_mode::vblank:
		{
		
			// 153 ly zero read
			if(current_line == 153 && scanline_counter < 4)
			{
				return 4 - scanline_counter;
			}
	
			return LINE_END - scanline_counter;
		}
	}
	//unreached
	return 0;
}

void Ppu::insert_new_ppu_event() noexcept
{
	const auto cycles = get_next_ppu_event();
	//printf("ppu_cyc: %d\n",cycles);
/*
	printf("event insertion %d:%d:%d:%d\n",static_cast<int>(mode),scanline_counter,current_line,cycles);
	if(cycles < 0)
	{
		printf("negative cycles %d:%d:%d\n",static_cast<int>(mode),scanline_counter,current_line);
		exit(1);
	}
*/
	const auto event = scheduler.create_event(cycles << cpu.get_double(),gameboy_event::ppu);
	scheduler.insert(event,false); 
}





// cgb
void Ppu::set_bg_pal_idx(uint8_t v) noexcept
{
	bg_pal_idx = v & 0x3f;
}

void Ppu::set_sp_pal_idx(uint8_t v) noexcept
{
	sp_pal_idx = v & 0x3f;
}

void Ppu::write_sppd(uint8_t v) noexcept
{
	// cant be accessed during pixel transfer
	if(mode != ppu_mode::pixel_transfer)
	{
		sp_pal[sp_pal_idx] = v; 
	}

	if(is_set(mem.io[IO_SPPI],7)) // increment on a write 
	{
		sp_pal_idx = (sp_pal_idx + 1) & 0x3f;
		mem.io[IO_SPPI] &= ~0x3f;
		mem.io[IO_SPPI] |= sp_pal_idx;
	}	
}

void Ppu::write_bgpd(uint8_t v) noexcept
{
	// cant be accessed during pixel transfer
	if(mode != ppu_mode::pixel_transfer)
	{
		bg_pal[bg_pal_idx] = v; 
	}

	if(is_set(mem.io[IO_BGPI],7)) // increment on a write 
	{
		bg_pal_idx = (bg_pal_idx + 1) & 0x3f;
		mem.io[IO_BGPI] &= ~0x3f;
		mem.io[IO_BGPI] |= bg_pal_idx;
	}	
}


// cant access either during pixel transfer

uint8_t Ppu::get_sppd() const noexcept
{
	if(mode != ppu_mode::pixel_transfer)
	{
		return sp_pal[sp_pal_idx];
	}

	return 0xff;
}

uint8_t Ppu::get_bgpd() const noexcept
{
	if(mode != ppu_mode::pixel_transfer)
	{
		return bg_pal[bg_pal_idx];
	}

	return 0xff;	
}


ppu_mode Ppu::get_mode() const noexcept
{

	// reads hblank in glitched oam mode
	if(glitched_oam_mode)
	{
		return ppu_mode::hblank;
	}
	return mode;
}

void Ppu::write_stat() noexcept
{
	// stat write glitch
	// behaves as if 0xff for 1 cycle on dmg
	if(!cpu.get_cgb())
	{
		const auto backup = mem.io[IO_STAT];
		mem.io[IO_STAT] = 0xff;
		stat_update();
		mem.io[IO_STAT] = backup;
	}

	stat_update();
}


uint8_t Ppu::read_stat() const noexcept
{
	return (mem.io[IO_STAT] & ~3) | static_cast<int>(get_mode());
}

// mode change, write, or line change
// will trigger this
void Ppu::stat_update() noexcept
{
	// if the lcd is not on then dont do anything
	if(!mem.is_lcd_enabled())
	{
		return;
	}

	// read stat and mask the mode
	uint8_t status = mem.io[IO_STAT];

	// save our current signal state
	const bool signal_old = signal;



	// check coincidence  (lyc == ly)
	// if lyc is current line set coincidence bit else deset it
	// line 153 acts as line zero 4 cycles after line start
	const bool lyc_hit = mem.io[IO_LYC] == get_current_line();
	status = lyc_hit? set_bit(status,2) : deset_bit(status,2);

	const bool lyc_signal = is_set(status,6) && lyc_hit;


	// not really sure what stat intrs should do after a lcd on
	if(!glitched_oam_mode)
	{
		// check nterrupts
		// stat irq interrupt reqed when we go from no stat conditions met
		// to any stat condition met
		switch(mode)
		{
			case ppu_mode::hblank: signal = is_set(status,3) || lyc_signal; break;
			case ppu_mode::vblank: signal = is_set(status,4) || lyc_signal; break;
			case ppu_mode::oam_search: signal = is_set(status,5) || lyc_signal; break;
			case ppu_mode::pixel_transfer: signal = lyc_signal; break;
		}
	}

	else
	{
		signal = lyc_signal;
	}

	// if we have changed from 0 to 1 for signal(signal edge)
	// request a stat interrupt
	if(!signal_old && signal)
	{
		cpu.request_interrupt(1);	
	}
	

	mem.io[IO_STAT] = status | 128;
}


// cant really find much information on the lcd on off behavior....

void Ppu::turn_lcd_off() noexcept
{
	// i think the behavior is this but im honestly not sure
	// read hblank during lcd off
	mode = ppu_mode::hblank;

	// what should happen to the interrupt signal when the lcd goes off?


	// this just gets shut off dont tick it
	scheduler.remove(gameboy_event::ppu,false);
	early_line_zero = false;

	window_y_triggered = false;
	window_x_triggered = false;

	scanline_counter = 0; // counter is reset?
	current_line = 0; // reset ly
}

// 1st line after this turns on oam search will fail
// and stat return as if its in hblank?
void Ppu::turn_lcd_on() noexcept
{
	//printf("lcd enabled %x\n",mem.is_lcd_enabled());
	//printf("%x:%x:%x\n",scanline_counter,current_line,mem.io[IO_LYC]);

	// again not sure what the behavior is when the ppu goes back on
	mode = ppu_mode::oam_search;
	stat_update();

	// i think it should read hblank till it hits pixel xfer
	// and allow writes through but im honestly not sure
	// blargss lcd-sync test is probably what we need...

	//printf("lyc bit %x\n",is_set(mem.io[IO_STAT],2));
	//printf("stat on: %x\n",mem.io[IO_STAT]);

	emulate_pixel_fifo = false;

	// oam fails to lock takes one less m cycle
	glitched_oam_mode = true;
	// needs verification
	scanline_counter = 4;

	insert_new_ppu_event();
}

void Ppu::window_disable() noexcept
{
	// window turned off midline need to disable it
	if(window_x_triggered && mode == ppu_mode::pixel_transfer)
	{
		reset_fetcher();
	}
}

void Ppu::switch_hblank() noexcept
{
	// switch to hblank
	mode = ppu_mode::hblank;

	// on cgb do hdma (handler will check if its active)
	if(cpu.get_cgb())
	{
		mem.do_hdma();
	}

	// if we draw the window at all this line 
	// we will draw it from a greater pos next line
	if(window_x_triggered)
	{
		window_y_line++;
		window_x_triggered = false;
		window_x_line = 0;
	}
	
	

	emulate_pixel_fifo = false;

	stat_update();
	insert_new_ppu_event();	

}

void Ppu::update_graphics(uint32_t cycles) noexcept
{

	// lcd is off nothing to do
	if(!mem.is_lcd_enabled()) // <-- should re enable on a delay?
	{
		return; 
	}

	//printf("cycles: %d\n",cycles);


	//-----------------------
	// update the stat reg state
	// and trigger interrupts	
	
	// read out current stat reg
	const uint8_t status = mem.io[IO_STAT];

	scanline_counter += cycles; // advance the cycle counter

	switch(mode)
	{	
		case ppu_mode::hblank: // hblank
		{
			if(scanline_counter >= LINE_END)
			{
				//printf("hblank ended at %d:%d\n",scanline_counter,current_line);
				// reset the counter extra cycles should tick over
				scanline_counter -= LINE_END;

				current_line++;
				
				if(current_line == 144)
				{
					mode = ppu_mode::vblank; // switch to vblank
					new_vblank = true;
					window_y_triggered = false;
					cpu.request_interrupt(0); // vblank interrupt
					
					// edge case oam stat interrupt is triggered here if enabled
					if(is_set(status,5) && !signal)
					{
						cpu.request_interrupt(1);
						signal = true;
					}				
				}
				
				else 
				{
					mode = ppu_mode::oam_search;
				}
				stat_update();
				insert_new_ppu_event();		
			}
			break;
		}
		
		
		case ppu_mode::vblank: // vblank
		{
			if(scanline_counter >= LINE_END)
			{
				//printf("vblank line ended at %d:%d\n",scanline_counter,current_line);
				scanline_counter -= LINE_END;
				current_line++;
				
				// vblank is over
				if(current_line > 153)
				{
					current_line = 0;
					window_y_line = 0;
					// enter oam search on the first line :)
					mode = ppu_mode::oam_search; 
					early_line_zero = false;				
				}
				stat_update();
				insert_new_ppu_event();		
			}

			// line 153 ly will read out zero after 4 cycles
			// this affects lyc intrs
			else if(current_line == 153 && !early_line_zero && scanline_counter >= 4)
			{
				early_line_zero = true;
				stat_update();
				insert_new_ppu_event();
			}

			break;
		}
		
		// mode 2 oam search
		case ppu_mode::oam_search:
		{
			// mode 2 takes four less cycles in glitched oam mode
			if(scanline_counter >= OAM_END)
			{
				glitched_oam_mode = false;

				if(current_line == mem.io[IO_WY])
				{
					window_y_triggered = true;
				}

				// switch to pixel transfer
				mode = ppu_mode::pixel_transfer;
				
				pixel_transfer_end = calc_pixel_transfer_end();
/*
				// if using fifo allways for testing
				reset_fetcher();
				scx_cnt = mem.io[IO_SCX] & 0x7;
*/

				// read in the sprites we are about to draw
				read_sprites();
				stat_update();
				insert_new_ppu_event();
			}
			break;
		}
		
		// pixel transfer
		case ppu_mode::pixel_transfer: 
		{

			if(emulate_pixel_fifo)
			{
				draw_scanline(cycles);
			}
			
			else if(scanline_counter >= pixel_transfer_end)
			{
				render_scanline();
				switch_hblank();
			}


/*	
			// testing just fetcher
			draw_scanline(cycles);
*/

/*
			// testing just scanline
			if(scanline_counter >= pixel_transfer_end)
			{
				render_scanline();
				switch_hblank();
			}
*/			
			break;
		}	
	}
}

void Ppu::ppu_write() noexcept
{
	// written during mid scanline
	// switch to using the fetcher 
	// and smash any cycles off
	if(mode == ppu_mode::pixel_transfer)
	{
		pixel_transfer_end = calc_pixel_transfer_end();

		//printf("%x:mid scaline!\n",cpu.get_pc()); exit(1);
		
		if(!emulate_pixel_fifo)
		{
			reset_fetcher();
			scx_cnt = mem.io[IO_SCX] & 0x7;
			// until we leave mode 3 remove it
			// has to be removed before we tell it its in the pixel fifo otherwhise
			// it might trigger hblank and have its event removed
			scheduler.remove(gameboy_event::ppu);
			emulate_pixel_fifo = true;
			draw_scanline(scanline_counter-OAM_END);
		}
	}
}

uint32_t Ppu::calc_pixel_transfer_end() noexcept
{
	// does not handle the sprite delay
	uint32_t cycles = 252; // base
	cycles += mem.io[IO_SCX] % 8; // scx delay

	// is the window drawn on this line?
	const bool window_rendered = mem.io[IO_WX] <= 166 && 
		window_y_triggered && is_set(mem.io[IO_LCDC],5);

	// verify window delay
	if(window_rendered && mem.io[IO_SCX] != 0)
	{
		cycles += 16;
	}

	return cycles;
}

uint32_t Ppu::get_dmg_color(int color_num, pixel_source source) const noexcept
{
	const auto source_idx = static_cast<int>(source);

	const int color_address = + source_idx + 0xff47;	
	const uint8_t palette = mem.io[color_address & 0xff];
	const int color_idx = (palette >> (color_num * 2)) & 3; 
	
	return 0xff000000 | dmg_pal[source_idx][color_idx];
}

uint32_t Ppu::get_cgb_color(int color_num, int cgb_pal, pixel_source source) const noexcept
{

	// each  rgb value takes two bytes in the pallete for cgb
	const int offset = (cgb_pal*8) + (color_num * 2); 

	uint16_t col;
	if(source <= pixel_source::tile)	
	{
		memcpy(&col,&bg_pal[offset],sizeof(col));
	}
	
	
	else // is a sprite
	{
		memcpy(&col,&sp_pal[offset],sizeof(col));		
	}

	// gameboy stores palletes in  format?
	int blue = col & 0x1f;
	int green = (col >> 5) & 0x1f;
	int red = (col >> 10) & 0x1f;
	
	// convert rgb15 to rgb888
	red = (red << 3) | (red >> 2);
	blue = (blue << 3) | (blue >> 2);
	green = (green << 3) | (green >> 2);


	const uint32_t full_color = blue | (green << 8) | (red << 16);

	return full_color | 0xff000000;
}



// considering taking copies and threading this
void Ppu::render_scanline() noexcept
{


    const bool is_cgb = cpu.get_cgb();
	const auto scx_offset = mem.io[IO_SCX] & 0x7;
	// is sprite drawing enabled?
	const bool obj_enabled = is_set(mem.io[IO_LCDC],1);
	

	// is the window drawn on this line?
	const bool window_rendered = mem.io[IO_WX] <= 166 && 
		window_y_triggered && is_set(mem.io[IO_LCDC],5);

	window_x_triggered = window_rendered;
	

	if(!window_rendered)
	{
		for(tile_cord = 0; tile_cord < 176; tile_cord += 8)
		{
			tile_fetch(&scanline_fifo[tile_cord],false);
		}
	}


	// window rendering
	else
	{

		// draw up to the window and then start re rendering from it 
		for(tile_cord = 0; tile_cord < mem.io[IO_WX]; tile_cord += 8)
		{
			tile_fetch(&scanline_fifo[tile_cord],false);
		}

		// is there a cleaner way to achieve this?
		const uint8_t win_offset = mem.io[IO_WX] < 7? 0 : mem.io[IO_WX] - 7;

		for(tile_cord = win_offset; tile_cord < 176; tile_cord += 8)
		{
			tile_fetch(&scanline_fifo[tile_cord+scx_offset],true);
		}
	}


	if(obj_enabled)
	{
		sprite_fetch(&scanline_fifo[scx_offset],false);
	}

	for(size_t x = 0; x < SCREEN_WIDTH; x++)
	{
		const auto pixel = scanline_fifo[x+scx_offset];

		if(!is_cgb)
		{
			const uint32_t full_color = get_dmg_color(pixel.colour_num,pixel.source);
			screen[(current_line*SCREEN_WIDTH)+x] = full_color;
		}
		
		else // gameboy color
		{
			const uint32_t full_color = get_cgb_color(pixel.colour_num, pixel.cgb_pal, pixel.source);
			screen[(current_line*SCREEN_WIDTH)+x] = full_color;
		}		
	}

}

// shift a pixel out of the array and smash it to the screen 
// increment x afterwards

// returns true if hblank has started

bool Ppu::push_pixel() noexcept
{

	// cant push anymore
	if(bg_fifo.len <= 8) 
	{ 
		return false; 
	}

	// ignore how much we are offset into the tile
	// if we fetched a bg tile
	// i highly doubt there is a hard coded dont do the shift is this aint the window
	// on real hardware i need to find out how this is actually done
	if(scx_cnt > 0 && !window_x_triggered)
	{


		// push a pixel out out
		bg_fifo.read_idx = (bg_fifo.read_idx + 1) % bg_fifo.size;
		bg_fifo.len--;

		scx_cnt--;
		return false;
	}

	// assume bg winns
	bool sprite_priority = false;

	const auto bg = bg_fifo.fifo[bg_fifo.read_idx];
	bg_fifo.read_idx = (bg_fifo.read_idx + 1) % bg_fifo.size;
	bg_fifo.len--;

	const auto sp = obj_fifo.fifo[obj_fifo.read_idx];

	// if sprite has stuff to push
	// advance the fifo and figure out which one wins
	if(obj_fifo.len != 0)
	{
		// "push" the sprite object out
		obj_fifo.read_idx = (obj_fifo.read_idx + 1) % obj_fifo.size;
		obj_fifo.len--;

		sprite_priority = sprite_win(sp,bg);
	}

	const auto pixel = sprite_priority? sp : bg;

	if(!cpu.get_cgb())
	{
		const uint32_t full_color = get_dmg_color(pixel.colour_num,pixel.source);
		screen[(current_line*SCREEN_WIDTH)+x_cord] = full_color;
	}
	
	else // gameboy color
	{
		const uint32_t full_color = get_cgb_color(pixel.colour_num, pixel.cgb_pal, pixel.source);
		screen[(current_line*SCREEN_WIDTH)+x_cord] = full_color;
	}
	
	x_cord += 1;
	if(x_cord == 160)
	{
		// done drawing enter hblank
		switch_hblank();
		return true;
	}

	return false;
}	
	





// todo proper scx and window timings
// as we current do not implement them at all 
// window should restart the fetcher when triggered 
// and take 6 cycles 

// fetcher operates at half of base clock
// therefore all cycle timings counts are doubled
// (as this just works out nicer than dividing the inputs by two)

// need to handle bugs with the window
void Ppu::tick_fetcher() noexcept
{

	// 1 cycle is tile num 
	// 2nd is lb of data 
	// 3rd is high byte of data 
	// 4th attempt to push 
	fetcher.cyc++; 
	
	// advance the fetcher if we dont have a tile dump waiting
	// fetcher operates at half of base clock (4mhz)
	// note atm this only fetches tiles but on real hardware we fetch sprites and tiles :D
	if(!fetcher.ready) // fetch some data
	{
		// should fetch the number then low and then high byte
		// but we will ignore this fact for now
		// takes 3 cycles to fetch 8 pixels
		if(fetcher.cyc >= 6) 
		{
			tile_fetch(fetcher.buf,window_x_triggered);
			fetcher.ready = true;
		}
	}
	
	// if we have room to dump into the fifo
	// and we are ready to do so, do it now 
	// at 0 dump at start at 8 pixels dump in higher half
	if(fetcher.ready)
	{
		if(bg_fifo.len <= 8)
		{
			for(int i = 0; i < 8; i++)
			{
				bg_fifo.fifo[bg_fifo.write_idx] = fetcher.buf[i];
				bg_fifo.write_idx = (bg_fifo.write_idx + 1) % bg_fifo.size;
			}
			tile_cord += 8; // goto next tile fetch
			bg_fifo.len += 8;
			fetcher.ready = false;
			fetcher.cyc = 0;
		}	
	}		
}	
	

void Ppu::draw_scanline(uint32_t cycles) noexcept 
{
	// is sprite drawing enabled?
	const bool obj_enabled = is_set(mem.io[IO_LCDC],1);
	

	// advance the fetcher and the fifo
	for(uint32_t i = 0; i < cycles; i++) // 1 pixel pushed per cycle
	{
	
		// just started drawing window
		// reset bg fifo and fetcher
		// ideally id cache this for the draw
		const bool using_window = window_active();
		if(!window_x_triggered && using_window)
		{
			window_x_triggered = true;
			bg_fifo.reset();
			fetcher.reset();
		}
		
	
		// ignore sprite timings for now
		// sprites are fetched instantly into the fifo
		// and not into the fetcher as they should be
		if(obj_enabled)
		{
			sprite_fetch(obj_fifo.fifo);
		}

		tick_fetcher();

		// blit the pixel
		// fifo will check if it can push for us
		const auto hblank = push_pixel();

		// if in hblank there is no more to do 
		if(hblank)
		{
			return;
		}
	}
}


bool Ppu::window_active() const noexcept
{

	// is the window enabled check in lcd control
	// and the window wy was == ly at some point during the frame
	// note if it is swapped during the frame to an ealier pos it wont trigger
	// see exponents test :P
	// and our draw cord is past wx?
	const bool using_window = is_set(mem.io[IO_LCDC],5) && window_y_triggered 
		&&  (mem.io[IO_WX] <= 166) && (mem.io[IO_WX] <= x_cord + 7);	

	return using_window;
}

// fetch a single tile into the fifo

void Ppu::tile_fetch(Pixel_Obj *buf, bool use_window) noexcept
{

    const bool is_cgb = cpu.get_cgb();
	const uint8_t lcd_control = mem.io[IO_LCDC];

	use_window = use_window && is_set(mem.io[IO_LCDC],5);

	// in dmg mode bg and window lose priority
	// if lcdc bit 0 is reset
	if(!is_cgb && !is_set(lcd_control,0)) 
	{
		// each pixel uses color 0 of bgp
		for(int i = 0; i < 8; i++)
		{
			fetcher.buf[i].colour_num = 0;
			fetcher.buf[i].source = pixel_source::tile;
		}
		return;
	}

	// where to draw the visual area and window
	const int scanline = current_line;
	
	// which of the 32 horizontal tiles are we currently drawing
	uint8_t x_pos = tile_cord;

	// ypos is used to calc which of the 32 vertical tiles 
	// the current scanline is drawing	
	uint8_t y_pos = scanline;

	unsigned int background_mem = 0;

	// which background mem?
	if(!use_window)
	{
		background_mem = is_set(lcd_control,3) ? 0x1c00 : 0x1800;
		y_pos += mem.io[IO_SCY];
		x_pos += mem.io[IO_SCX];
	}
	
	else
	{
		// which window mem?
		background_mem = is_set(lcd_control,6) ? 0x1c00 : 0x1800;
		y_pos = window_y_line;
		x_pos = window_x_line;

		window_x_line += 8;
	}


	
	// which of the 8 vertical pixels of the scanline are we on
	// y pos is currently the raw offset in pixels into the bg map
	// we need to / 8 to scale it to how rows down it is
	// then times it by 32 (bg map is 32 by 32)
	const int tile_row = ((y_pos / 8) & 31) * 32;

	// same here limit tile index to 31 so it wraps around
	const int tile_col = (x_pos / 8) & 31;
 
	
	

	// get the tile identity num it can be signed or unsigned
	// -0x8000 to account for the vram 
	const int tile_address = background_mem + tile_row + tile_col;

	// deduce where this tile identifier is in memory
	int tile_location = 0;

	
	// tile number is allways bank 0
	if(is_set(lcd_control,4)) // unsigned
	{
		const auto tile_num = mem.vram[0][tile_address];
		tile_location = tile_num * 16;
	}
	
	else // signed tile index 0x1000 is used as base pointer relative to start of vram
	{
		const auto tile_num = static_cast<int8_t>(mem.vram[0][tile_address]);
		tile_location = 0x1000 + (tile_num * 16);
	}

	int cgb_pal = -1;
	bool priority = false;
	bool x_flip = false;
	bool y_flip = false;
	int vram_bank = 0;


	if(is_cgb) // we are drawing in cgb mode 
	{
		// bg attributes allways in bank 1
		const uint8_t attr = mem.vram[1][tile_address];
		cgb_pal = attr & 0x7; // get the pal number
				
				
		// draw over sprites
		priority = is_set(attr,7);
		x_flip = is_set(attr,5);
		y_flip = is_set(attr,6);

		// decide what bank data is coming out of
		// allready one so dont check the other condition
		vram_bank = is_set(attr,3) ? 1 : 0;
	}

	y_pos &= 7;  // scale to line on the tile ( a tile is 8 pixels high)

	// find the correct vertical line we are on of the
	// tile to get the tile data		
	// read the sprite backwards in y axis if y flipped
	// must be times by 2 as each line takes up 2 bytes
	const unsigned int line = y_flip? (7 - y_pos) * 2 : y_pos*2;
		
			
	const uint8_t data1 = mem.vram[vram_bank][tile_location+line];
	const uint8_t data2 = mem.vram[vram_bank][tile_location+line+1];
	
	// pixel 0 in the tile is bit 7 of data1 and data2
	// pixel 1 is bit 6 etc
	
	unsigned int color_bit = x_flip? 0 : 7;
	
	const int shift = x_flip ? 1 : -1;

	// in cgb an priority bit is set it has priority over sprites
	// unless lcdc has the master overide enabled
	const auto source = priority ? pixel_source::tile_cgbd : pixel_source::tile;	

	for(unsigned int i = 0; i < 8; i++, color_bit += shift)
	{
		// combine data 2 and data 1 to get the color id for the pixel
		// in the tile
		int colour_num = is_set(data2,color_bit) << 1;
		colour_num |= is_set(data1,color_bit);
			
			
		// save our info to the fetcher
		// in dmg the pal number will be ignored
		buf[i].cgb_pal = cgb_pal;

		buf[i].colour_num = colour_num;
		buf[i].source = source;	
	}
}


// read the up to 10 sprites for the scanline
// called when when enter pixel transfer
void Ppu::read_sprites() noexcept
{
	// priority
	// by oam index if in cgb mode
	// in dmg by xcord unless equal where it 
	// is also done by oam index



	const uint8_t lcd_control = mem.io[IO_LCDC]; // get lcd control reg

	const int y_size = is_set(lcd_control,2) ? 16 : 8;
	
	const uint8_t scanline = current_line;

	
	// reset how many sprites we have as its a new scanline
	no_sprites = 0;
	cur_sprite = 0;
	for(size_t sprite = 0; sprite < 40; sprite++) // should fetch all these as soon as we leave oam search
	{
        const uint16_t addr = sprite*4;
		const uint8_t y_pos = mem.oam[addr & 0xff];
		if( scanline -(y_size - 16) < y_pos  && scanline + 16 >= y_pos )
		{
			// intercepts with the line
			objects[no_sprites].index = addr; // save the index
			// and the x pos
			objects[no_sprites].x_pos = mem.oam[(addr+1)&0xff];
			objects[no_sprites].attr = mem.oam[(addr+3)];

			// priority by oam index in cgb update here
			if(cpu.get_cgb())
			{
				objects[no_sprites].priority = no_sprites;
			}
			if(++no_sprites == 10) { break; } // only draw a max of 10 sprites per line
		}
	}


	// in dmg by xcord unless equal where it 
	// is also done by oam index
	// we sort like this here regardless because we want them in xcord order
	// i.e the order our screen will draw them left to right
	// so we dont need rechecks
	std::sort(&objects[0],&objects[no_sprites],
	[](const Obj &sp1, const Obj &sp2)
	{
		if(sp1.x_pos == sp2.x_pos)
		{
			return sp1.index < sp2.index;
		}

		return sp1.x_pos < sp2.x_pos;
	});


	// in dmg we need to update the priority by here instead as it
	// is now in correct order 
	// todo respect opri
	if(!cpu.get_cgb())
	{
		for(size_t i = 0; i < no_sprites; i++)
		{
			objects[i].priority = i;
		}
	}
}




bool Ppu::sprite_win(const Pixel_Obj &sp, const Pixel_Obj &bg) noexcept
{

	// in cgb if lcdc bit 0 is deset sprites draw over anything
	const bool draw_over_everything = !is_set(mem.io[IO_LCDC],0) && cpu.get_cgb();

	// dont display pixels with colour id zero as its allways transparent
	// the colour itself dosent matter we only care about the id
	if(sp.colour_num == 0)
	{
		return false;
	}

	// if the cgb lcdc sprite overrride is on then the sprite will win every time
	// also if the tile is color zero (transparent) then the sprite will also win
	if(!draw_over_everything && bg.colour_num != 0)
	{
		
		// cgb tile attr set to priority
		// oam obj above bg ignored
		if(bg.source == pixel_source::tile_cgbd)
		{
			return false;
		}

		// oam priority bit set tile is above sprite
		else if(is_set(objects[sp.sprite_idx].attr,7))
		{
			return false;
		}
	}

	return true;	
}


// the buf passed in current goes unused if doing fifo rendering
// when we move this to use the actual fetcher it will used a pass param
// of it instead of hardcoding a direct dump into the fifo
void Ppu::sprite_fetch(Pixel_Obj *buf,bool use_fifo) noexcept
{
    const bool is_cgb = cpu.get_cgb();
	
	const uint8_t lcd_control = mem.io[IO_LCDC]; // get lcd control reg

	const int y_size = is_set(lcd_control,2) ? 16 : 8;

	const int scanline = current_line;
		
	for(size_t i = cur_sprite; i < no_sprites; i++)
	{

		// TODO figure out a more sensible way to handle sprites < 8	
		// this is probably related to the reason the documentation
		// says the oam entry is the x pos - 8

		// offset into the sprite data we start drawing at 
		// 7 is the 0th pixel and is defualt for a 
		// sprite that we draw fully
		int pixel_start = 7; 

		uint8_t x_pos = objects[cur_sprite].x_pos;

		if(use_fifo)
		{
			// sprite < 8
			if(x_cord == 0 && x_pos < 8)
			{
				// here because the sprite gets -8
				// this means it would normally underflow
				// and only draw how much it is offset
				// on the screen so account for the pixel start
				pixel_start = x_pos;
			}


			// if it does not start at the current x cord 
			// then we dont care
			// we do + 8 instead of - 8 for the x_pos to avoid
			// underflows
			else if(x_pos != x_cord + 8)
			{
				continue;
			}
		}

		// scanline renderer so we are just allways drawing the thing if its on screen
		else
		{
			// dont draw out of range sprites :P
			if(x_pos >= SCREEN_WIDTH+8)
			{
				continue;
			}

			// just draw from zero up to the posistion
			if(x_pos < 8)
			{
				pixel_start = objects[cur_sprite].x_pos;
				x_pos = 0;
			}

			// draw from -8 to the posistion
			else
			{
				x_pos -= 8;
			}
		}

		
		
		
		// sprite takes 4 bytes in the sprite attributes table
		const uint8_t sprite_index = objects[i].index;
		uint8_t y_pos = mem.oam[sprite_index];
		// lowest bit of tile index ignored for 16 pixel sprites
		const uint8_t sprite_location = y_size == 16? mem.oam[(sprite_index+2)] & ~1 : mem.oam[(sprite_index+2)];
		const uint8_t attributes = mem.oam[(sprite_index+3)];
		
		const bool y_flip = is_set(attributes,6);
		const bool x_flip = is_set(attributes,5);


		
		// if this sprite doesent meet the scanline we dont care
		if(!( scanline -(y_size - 16) < y_pos  && scanline + 16 >= y_pos ))
		{
			continue;
		}
		
		y_pos -= 16;
		uint8_t line = scanline - y_pos; 
		
		// read the sprite backwards in y axis
		if(y_flip)
		{
			line = y_size - (line + 1);
		}
		
		line *= 2; // each line of sprite data is two bytes
		uint16_t data_address = ((sprite_location * 16 )) + line; // in realitly this is offset into vram at 0x8000

		// if in cgb and attr has bit 3 set 
		// read from the 2nd vram bank
		const int vram_bank = (is_cgb && is_set(attributes,3))? 1 : 0;

		const uint8_t data1 = mem.vram[vram_bank][data_address];
		const uint8_t data2 = mem.vram[vram_bank][data_address+1];
		

		// if xflipped we need to read from start to end
		// and else end to start (see below)
		int colour_bit = x_flip? 7 - pixel_start : pixel_start;
		const int shift = x_flip? 1 : -1;

		const auto source = is_set(attributes,4)? pixel_source::sprite_one : pixel_source::sprite_zero;

		// render into the fifo
		if(use_fifo)
		{
			// eaiser to read in from right to left as pixel 0
			// is bit 7 in the color data pixel 1 is bit 6 etc 
			for(int sprite_pixel = pixel_start; sprite_pixel >= 0; sprite_pixel--,colour_bit += shift)
			{

				// rest same as tiles
				const int colour_num = (is_set(data2,colour_bit) << 1) 
					| is_set(data1,colour_bit);

				// where we actually want to dump the pixel into the fifo
				const size_t x_pix = pixel_start - sprite_pixel;


				// for now we are just writing directly into the fifo and not the fetcher
				// and completing fetches instantly this is not how hardware works but just go with it for now
				auto &fifo_ref = obj_fifo.fifo[(x_pix + obj_fifo.read_idx) % obj_fifo.size];

				/* 
					lower objects idx means that a fetcher object has a greater priority
					on dmg this will just work as priority is by x cordinate and therefore 
					the order sprites will be drawn in thus the fifo will fill up with
					higher priority sprites first and deny sprites that draw after it
					however on cgb they will draw out of priority order as its by oam idx
					so we need a manual check by here 
				*/

				// if a pixel is allready in the fifo
				if(x_pix < obj_fifo.len)
				{
					// current color is transparent and a pixel 
					// is allready there we lose
					if(colour_num == 0)
					{
						continue;
					}

					// pixel allready there has a higher priority
					// and color is not transparent we lose
					else if(objects[fifo_ref.sprite_idx].priority < i && fifo_ref.colour_num != 0)
					{
						continue;
					}
				}	


				fifo_ref.colour_num = colour_num;
				fifo_ref.source = source;
				fifo_ref.sprite_idx = cur_sprite;

				// value just ignored in dmg
				fifo_ref.cgb_pal = attributes & 0x7;	 
			}
			// update the len if we have added new pixels :P
			// and not just mixed old ones
			obj_fifo.len = std::max(static_cast<size_t>(pixel_start+1),obj_fifo.len);
			cur_sprite += 1;
		}


		// scanline renderer
		else
		{
			for(int sprite_pixel = pixel_start; sprite_pixel >= 0; sprite_pixel--,colour_bit += shift)
			{
				// rest same as tiles
				const int colour_num = (is_set(data2,colour_bit) << 1) 
					| is_set(data1,colour_bit);


				// where we actually want to dump the pixel into the fifo
				const size_t x_pix = pixel_start - sprite_pixel;

				// just dump the thing directly we only need to check tile to sprite wins here
				// as lower priority ones will just get drawn over when we are using the painters algorithm
				auto &pixel_ref = buf[x_pix + x_pos];

				Pixel_Obj sp;
				sp.colour_num = colour_num;
				sp.source = source;
				sp.sprite_idx = cur_sprite;
				sp.cgb_pal = attributes & 0x7;

				bool is_tile = (pixel_ref.source <= pixel_source::tile);

				// if a tile is there check which has priority
				if(is_tile)
				{
					if(!sprite_win(sp,pixel_ref))
					{
						continue;
					}
				}

				// is a sprite check if we lose
				// if we are transparent 
				// or of a lower priority while the one allready there 
				// is transparent we lose
				else
				{
					if(colour_num == 0)
					{
						continue;
					}

					else if(objects[pixel_ref.sprite_idx].priority < i && pixel_ref.colour_num != 0)
					{
						continue;
					}
				}


				pixel_ref = sp;
			}
			cur_sprite += 1;				
		}	
	}
}

}