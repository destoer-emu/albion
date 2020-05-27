#include <gb/gb.h>


// regressions with timigns tests after rewrite
// and even when hard coding the minimum pixel transfer time
// for scanline rendering
// which should make no difference as we 
// didnt handle sprite and win stalls in the fifo anyways...
// seems to cause 2 of the ppu tests to fail?
// need to look into this

namespace gameboy
{

Ppu::Ppu(GB &gb) : cpu(gb.cpu), mem(gb.mem) 
{
	screen.resize(SCREEN_WIDTH*SCREEN_HEIGHT);
	std::fill(screen.begin(),screen.end(),0);	
}

void Ppu::reset_fetcher() noexcept
{
	x_cord = 0; // current x cord of the ppu
	tile_cord = 0;
	fetcher.ready = false; 
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

	reset_fetcher();

    // cgb pal
	sp_pal_idx = 0;
	bg_pal_idx = 0; // index into the bg pal (entry takes two bytes)

	window_y_line = 0;

	memset(bg_pal,0x00,sizeof(bg_pal)); // bg palette data
	memset(sp_pal,0x00,sizeof(sp_pal)); // sprite pallete data 

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
	return mode;
}

void Ppu::write_stat() noexcept
{
	stat_update();
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
	status &= ~0x3;

	// save our current signal state
	const bool signal_old = signal;



	// check coincidence  (lyc == ly)
	// if lyc is current line set coincidence bit else deset it
	bool lyc_hit = mem.io[IO_LYC] == current_line;
	status = lyc_hit? set_bit(status,2) : deset_bit(status,2);

	bool lyc_signal = is_set(status,6) && lyc_hit;


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
	

	// if we have changed from 0 to 1 for signal(signal edge)
	// request a stat interrupt
	if(!signal_old && signal)
	{
		cpu.request_interrupt(1);	
	}
	

	mem.io[IO_STAT] = status | 128 | static_cast<uint8_t>(mode);
}


// cant really find much information on the lcd on off behavior....

void Ppu::turn_lcd_off() noexcept
{
	scanline_counter = 0; // counter is reset?
	current_line = 0; // reset ly

	// i think the behavior is this but im honestly not sure
	mode = ppu_mode::hblank;
	mem.io[IO_STAT] = (mem.io[IO_STAT] & ~3) | static_cast<uint8_t>(mode);
	signal = false;
}


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
}

void Ppu::window_disable() noexcept
{

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
	if(window_drawn)
	{
		window_y_line++;
	}

	window_drawn = false;
	window_x_line = 0;

	stat_update();	
}

void Ppu::update_graphics(int cycles) noexcept
{

	// lcd is off nothing to do
	if(!mem.is_lcd_enabled()) // <-- should re enable on a delay?
	{
		return; // can exit if ppu is disabled nothing else to do
	}


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
				// reset the counter extra cycles should tick over
				scanline_counter -= LINE_END;

				current_line++;
				
				if(current_line == 144)
				{
					mode = ppu_mode::vblank; // switch to vblank
					new_vblank = true;
					cpu.request_interrupt(0); // vblank interrupt
					
					// edge case oam stat interrupt is triggered here if enabled
					if(is_set(status,5))
					{
						if(signal == false)
						{
							cpu.request_interrupt(1);
							signal = true;
						}	
					}				
				}
				
				else 
				{
					mode = ppu_mode::oam_search;
				}
				stat_update();		
			}
			break;
		}
		
		
		case ppu_mode::vblank: // vblank
		{
			if(scanline_counter >= LINE_END)
			{
				scanline_counter -= LINE_END;
				current_line++;
				
				// vblank is over
				if(current_line > 153)
				{
					current_line = 0;
					window_y_line = 0;
					// enter oam search on the first line :)
					mode = ppu_mode::oam_search; 				
				}
				stat_update();		
			}
			break;
		}
		
		// mode 2 oam search
		case ppu_mode::oam_search:
		{
			if(scanline_counter >= OAM_END)
			{
				// switch to pixel transfer
				mode = ppu_mode::pixel_transfer;
				
				emulate_pixel_fifo = false;
				pixel_transfer_end = calc_pixel_transfer_end();

				// if using fifo allways for testing
				//reset_fetcher();
				//scx_cnt = mem.io[IO_SCX] & 0x7;

				// read in the sprites we are about to draw
				read_sprites();
				stat_update();				
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
			
			else
			{
				if(scanline_counter >= pixel_transfer_end)
				{
					render_scanline();
					switch_hblank();
				}
			}
			
			// testing
			//draw_scanline(cycles);
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
		if(!emulate_pixel_fifo)
		{
			reset_fetcher();
			scx_cnt = mem.io[IO_SCX] & 0x7;
			emulate_pixel_fifo = true;
			draw_scanline(scanline_counter-OAM_END);
		}
	}
}

uint32_t Ppu::calc_pixel_transfer_end() noexcept
{
	// does not handle the sprite, window, scx
	// stalls and just assumes the minimum for now
	return 252;
}

uint32_t Ppu::get_dmg_color(int color_num, pixel_source source) noexcept
{
	int colour_address = static_cast<uint16_t>(source) + 0xff47;	
	dmg_colors col = get_colour(color_num,colour_address); 

	// black is default
	uint32_t full_color = 0xff000000;

	switch(col)
	{
		case dmg_colors::black: /*full_color = 0xff000000;*/ break;
		case dmg_colors::white: full_color = 0xffffffff; break;
		case dmg_colors::light_gray: full_color = 0xffcccccc;  break;
		case dmg_colors::dark_gray: full_color = 0xff777777; break;
	}

	return full_color;	
}

uint32_t Ppu::get_cgb_color(int color_num, int cgb_pal, pixel_source source) noexcept
{

	// each  rgb value takes two bytes in the pallete for cgb
	const int offset = (cgb_pal*8) + (color_num * 2); 

	int col;
	if(source == pixel_source::tile ||
		source == pixel_source::tile_cgbd)
	{
		col = bg_pal[offset];
		col |= bg_pal[offset + 1] << 8;
	}
	
	
	else // is a sprite
	{
		col = sp_pal[offset];
		col |= sp_pal[offset + 1] << 8;			
	}
	

	// gameboy stores palletes in bgr format?
	int blue = col & 0x1f;
	int green = (col >> 5) & 0x1f;
	int red = (col >> 10) & 0x1f;
	
	// convert rgb15 to rgb888
	red = (red << 3) | (red >> 2);
	blue = (blue << 3) | (blue >> 2);
	green = (green << 3) | (green >> 2);


	uint32_t full_color = blue;
	full_color |= green << 8;
	full_color |= red << 16;

	return full_color | 0xff000000;
}




void Ppu::render_scanline() noexcept
{


    const bool is_cgb = cpu.get_cgb();
	auto scx_offset = mem.io[IO_SCX] & 0x7;
	// is sprite drawing enabled?
	const bool obj_enabled = is_set(mem.io[IO_LCDC],1);

	Pixel_Obj scanline_fifo[176];

	// this will just render nicely without modifying it
	for(tile_cord = 0; tile_cord < 176; tile_cord += 8)
	{
		tile_fetch(&scanline_fifo[tile_cord]);
	}

	// window does not scroll
	// we many have to handle it triggering in the middle of the line
	if(window_drawn)
	{
		scx_offset = 0;
	}

	if(obj_enabled)
	{
		//printf("rendering sprites %d\n",no_sprites);
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
	if(scx_cnt > 0 && !window_drawn)
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
	

	if(++x_cord == 160)
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
	// 4th is pushing it into the fifo	
	fetcher.cyc += 1; // further along 
	
	// advance the fetcher if we dont have a tile dump waiting
	// fetcher operates at half of base clock (4mhz)
	// note atm this only fetches tiles but on real hardware we fetch sprites and tiles :D
	if(!fetcher.ready) // fetch some data
	{
		// should fetch the number then low and then high byte
		// but we will ignore this fact for now
		if(fetcher.cyc >= 6) // takes 3 cycles to fetch 8 pixels
		{
			tile_fetch(fetcher.buf);
			fetcher.ready = true;
		}
		return;
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
	

void Ppu::draw_scanline(int cycles) noexcept 
{
	// is sprite drawing enabled?
	const bool obj_enabled = is_set(mem.io[IO_LCDC],1);
	
	
	// advance the fetcher and the fifo
	for(int i = 0; i < cycles; i++) // 1 pixel pushed per cycle
	{
		// ignore sprite timings for now
		// sprites are fetched instantly into the fifo
		// and not into the fetcher as they should be
		if(obj_enabled)
		{
			sprite_fetch(obj_fifo.fifo);
		}
	
		// blit the pixel
		// fifo will check if it can push for us
		const auto hblank = push_pixel();

		// if in hblank there is no more to do 
		if(hblank)
		{
			return;
		}

		tick_fetcher();
	}
}

// fetch a single tile into the fifo

void Ppu::tile_fetch(Pixel_Obj *buf) noexcept
{

    const bool is_cgb = cpu.get_cgb();
	const uint8_t lcd_control = mem.io[IO_LCDC];

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
	const uint8_t scroll_y = mem.io[IO_SCY];
	const uint8_t scroll_x = mem.io[IO_SCX];
	const uint8_t window_y = mem.io[IO_WY];
	const uint8_t window_x = mem.io[IO_WX]; 
	const int scanline = current_line;
	

	// is the window enabled check in lcd control
	// and is the current scanline the window pos?
	// if we are using window 
	// it starts drawing at window_x 
	// so if we are less than it dont draw 
	const bool using_window = is_set(lcd_control,5) && (window_y <= scanline) 
		&&  (window_x <= 166) && (mem.io[IO_WX] <= tile_cord + 7);
	
	// which of the 32 horizontal tiles are we currently drawing
	uint8_t x_pos = tile_cord;

	// ypos is used to calc which of the 32 vertical tiles 
	// the current scanline is drawing	
	uint8_t y_pos = scanline;

	int background_mem = 0;

	// which background mem?
	if(!using_window)
	{
		background_mem = is_set(lcd_control,3) ? 0x1c00 : 0x1800;
		y_pos += scroll_y;
		x_pos += scroll_x;
	}
	
	else
	{
		// which window mem?
		background_mem = is_set(lcd_control,6) ? 0x1c00 : 0x1800;
		y_pos = window_y_line;
		x_pos = window_x_line;


		// cache if we have drawn the window on this line 
		window_drawn = true;
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
	const int line = y_flip? (7 - y_pos) * 2 : y_pos*2;
		
			
	const uint8_t data1 = mem.vram[vram_bank][tile_location+line];
	const uint8_t data2 = mem.vram[vram_bank][tile_location+line+1];
	
	// pixel 0 in the tile is bit 7 of data1 and data2
	// pixel 1 is bit 6 etc
	
	int color_bit = x_flip? 0 : 7;
	
	const int shift = x_flip ? 1 : -1;

	// in cgb an priority bit is set it has priority over sprites
	// unless lcdc has the master overide enabled
	const auto source = priority ? pixel_source::tile_cgbd : pixel_source::tile;	

	for(int i = 0; i < 8; i++, color_bit += shift)
	{
		// combine data 2 and data 1 to get the color id for the pixel
		// in the tile
		int colour_num = val_bit(data2,color_bit) << 1;
		colour_num |= val_bit(data1,color_bit);
			
			
		// save our info to the fetcher
		// in dmg the pal number will be ignored
		buf[i].cgb_pal = cgb_pal;

		buf[i].colour_num = colour_num;
		buf[i].source = source;	
	}
}

dmg_colors Ppu::get_colour(uint8_t colour_num, uint16_t address) noexcept
{
	const uint8_t palette = mem.io[address & 0xff];

	const int colour = (palette >> (colour_num * 2)) & 3; 

	static constexpr dmg_colors colors[] = {dmg_colors::white,dmg_colors::light_gray,
        dmg_colors::dark_gray,dmg_colors::black};

	return colors[colour];
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
				const int colour_num = (val_bit(data2,colour_bit) << 1) 
					| val_bit(data1,colour_bit);

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
				const int colour_num = (val_bit(data2,colour_bit) << 1) 
					| val_bit(data1,colour_bit);


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

				bool is_tile = (pixel_ref.source == pixel_source::tile || pixel_ref.source == pixel_source::tile_cgbd);

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



// -----------
// ppu viewer
// only used by the some frontends but its
// not code dependant on a frontend so its best placed here

std::vector<uint32_t> Ppu::render_bg(bool higher) noexcept
{

	std::vector<uint32_t> bg_map(256*256);

	const uint8_t lcd_control = mem.io[IO_LCDC]; // get lcd control reg
	int background_mem = higher ? 0x1c00 : 0x1800;

	bool is_cgb = cpu.get_cgb();

	// for each line
	for(int tile_y = 0; tile_y < 32; tile_y++)
	{
		// for each tile in the line
		for(int tile_x = 0; tile_x < 32; tile_x++)
		{
			const int bg_location =  background_mem + ((tile_y*32)+tile_x);

			int tile_location;

			// tile number is allways bank 0
			if(is_set(lcd_control,4)) // unsigned
			{
				const auto tile_num =  mem.vram[0][bg_location];
				tile_location = tile_num * 16;
			}
			
			else // signed tile index 0x1000 is used as base pointer relative to start of vram
			{
				const auto tile_num = static_cast<int8_t>(mem.vram[0][bg_location]);
				tile_location = 0x1000 + (tile_num * 16);
			}


			int buf_offset = (tile_y * 256 * 8) + (tile_x * 8);



			int cgb_pal = -1;
			bool x_flip = false;
			bool y_flip = false;
			int vram_bank = 0;


			if(is_cgb) // we are drawing in cgb mode 
			{
				// bg attributes allways in bank 1
				const uint8_t attr = mem.vram[1][bg_location];
				cgb_pal = attr & 0x7; // get the pal number
											
				x_flip = is_set(attr,5);
				y_flip = is_set(attr,6);

				// decide what bank data is coming out of
				// allready one so dont check the other condition
				vram_bank = is_set(attr,3) ? 1 : 0;
			}





			// for each line in the tile
			for(int y = 0; y < 8; y++)
			{

				const int line = y_flip? (7 - y) * 2 : y*2;

				// 2 bytes per line in vram
				uint16_t data_address = tile_location + line;

				const uint8_t data1 = mem.vram[vram_bank][data_address];
				const uint8_t data2 = mem.vram[vram_bank][data_address+1];


				int color_bit = x_flip? 0 : 7;
				
				const int shift = x_flip ? 1 : -1;

				// for each pixel in the line of the tile
				for(int x = 0; x < 8; x++, color_bit += shift)
				{

					// rest same as tiles
					int color_num = val_bit(data2,color_bit) << 1;
					color_num |= val_bit(data1,color_bit);

					if(!is_cgb)
					{
						const uint32_t full_color = get_dmg_color(color_num,pixel_source::tile);
						bg_map[buf_offset + (y * 256) + x] = full_color;
					}


					else
					{
						// dont care about the priority here just smash it
						const uint32_t full_color = get_cgb_color(color_num,cgb_pal, pixel_source::tile);
						bg_map[buf_offset + (y * 256) + x] = full_color;
					}
					
				}
			}
		}
	}



	// now render a black box over the viewing area

	const uint8_t scy = mem.io[IO_SCY];
	const uint8_t scx = mem.io[IO_SCX];

	// draw x bounds
	// draw two vertical lines to indicate scx start
	// and scx + screen width
	for(uint32_t y = 0; y < SCREEN_HEIGHT; y++)
	{
		const int y_offset = ((scy + y) & 0xff) * 256;

		bg_map[scx + y_offset] = 0xff0000ff;
		bg_map[((scx + SCREEN_WIDTH) & 0xff) +  y_offset] = 0xff0000ff;
	}

	// draw y bounds
	// draw two horizontal lines to indicate scy start
	// and scy + screen HEIGHT
	for(uint32_t x = 0; x < SCREEN_WIDTH; x++)
	{
		const int x_offset = (scx + x) & 0xff; 

		bg_map[(scy * 256) + x_offset] = 0xff0000ff;
		bg_map[(((scy + SCREEN_HEIGHT) & 0xff)*256) + x_offset] = 0xff0000ff;		
	}


	return bg_map;
}

void Ppu::render_palette(uint32_t *palette_bg,uint32_t *palette_sp) noexcept
{
	for(int cgb_pal = 0; cgb_pal < 8; cgb_pal++)
	{
		for(int color_num = 0; color_num < 4; color_num++)
		{
			palette_bg[(cgb_pal*4)+color_num] = get_cgb_color(color_num,cgb_pal, pixel_source::tile);
			palette_sp[(cgb_pal*4)+color_num] = get_cgb_color(color_num,cgb_pal,pixel_source::sprite_one);
		}
	}
}

std::vector<uint32_t> Ppu::render_tiles() noexcept
{

	std::vector<uint32_t> tiles(384*8*8*2);

	int banks = cpu.get_cgb()? 2 : 1;


	for(int bank = 0; bank < banks; bank++)
	{
		// 384 tiles total
		for(int tile_y = 0; tile_y < 0x18; tile_y++)
		{
			for(int tile_x = 0; tile_x < 0x10; tile_x++)
			{
				const int tile_num  = (tile_y * 0x10) + tile_x;

				// 0x80 bytes in a single line
				// however we do 8 at a time (8 by 8 tiles)
				// * 2 because there can be two banks
				const int buf_offset = (tile_y * 0x80 * 8 * 2) + (tile_x * 8) + (bank * 0x80);

				// for each line in the tile
				for(int y = 0; y < 8; y++)
				{

					// 2 bytes per line in vram
					const uint16_t data_address = (tile_num * 16) + (y * 2);

					const uint8_t data1 = mem.vram[bank][data_address];
					const uint8_t data2 = mem.vram[bank][data_address+1];

					// for each pixel in the line of the tile
					for(int x = 7; x >= 0; x--)
					{

						// rest same as tiles
						int color_num = val_bit(data2,x) << 1;
						color_num |= val_bit(data1,x);


						const uint32_t full_color = get_dmg_color(color_num,pixel_source::tile);
						tiles[buf_offset + (y * 0x80 * 2) + (7-x)] = full_color;
										
					}
				}		
			}
		}
	}

	return tiles;
}

}