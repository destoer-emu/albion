#include <gb/gb.h>

namespace gameboy
{

Ppu::Ppu(GB &gb) : cpu(gb.cpu), mem(gb.mem) 
{
	screen.resize(SCREEN_WIDTH*SCREEN_HEIGHT);
	std::fill(screen.begin(),screen.end(),0);	
}

void Ppu::reset_fetcher() noexcept
{
	// fetcher
	hblank = false;
	x_cord = 0; // current x cord of the ppu
	pixel_idx = 0;

	ppu_cyc = 0; // how far for a tile fetch is
	ppu_scyc = 0; // how far along a sprite fetch is
	pixel_count = 0; // how many pixels are in the fifo
	tile_cord = 0;
	tile_ready = false; // is the tile fetch ready to go into the fio 
	no_sprites = 0; // how many sprites
	sprite_drawn = false;
	window_start = false;
	x_scroll_tick = false;
	scx_cnt = 0;

	// if we draw the window at all this line 
	// we will draw it from a greater pos next line
	if(window_drawn)
	{
		window_y_line++;
	}

	window_drawn = false;
	window_x_line = 0;

	fetcher_sprite.reset();
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
	
	// update our status reg
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
	reset_fetcher();

	//printf("stat off: %x\n",mem.io[IO_STAT]);
}


void Ppu::turn_lcd_on() noexcept
{
	//printf("lcd enabled %x\n",mem.is_lcd_enabled());
	//printf("%x:%x:%x\n",scanline_counter,current_line,mem.io[IO_LYC]);

	// again not sure what the behavior is when the ppu goes back on
	mode = ppu_mode::oam_search;
	stat_update();

	//printf("lyc bit %x\n",is_set(mem.io[IO_STAT],2));
	//printf("stat on: %x\n",mem.io[IO_STAT]);
}

void Ppu::window_disable() noexcept
{

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
			if(scanline_counter >= 456)
			{
				// reset the counter extra cycles should tick over
				scanline_counter = 0;

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
			if(scanline_counter >= 456)
			{
				scanline_counter = 0;
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
			// mode 2 is over
			if(scanline_counter >= 80)
			{
				// switch to pixel transfer
				mode = ppu_mode::pixel_transfer;
				
				// read in the sprites we are about to draw
				read_sprites();

				scx_cnt = (mem.io[IO_SCX] & 0x7);
				x_scroll_tick = scx_cnt > 0;
				stat_update();				
			}
			break;
		}
		
		// pixel transfer
		case ppu_mode::pixel_transfer: 
		{
			draw_scanline(cycles);
			if(hblank) // if just entering hblank 
			{
				// switch to hblank
				mode = ppu_mode::hblank;
					
				reset_fetcher();
					
				// on cgb do hdma (handler will check if its active)
				if(cpu.get_cgb())
				{
					mem.do_hdma();
				}
				stat_update();
			}
			break;
		}	
	}
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



// shift a pixel out of the array and smash it to the screen 
// increment x afterwards

// returns wether it can keep pushing or not

bool Ppu::push_pixel() noexcept
{

	// cant push anymore
	if(!(pixel_count > 8)) { return false; }

	 // ignore how much we are offset into the tile
	 // if we fetched a bg tile
	if(x_scroll_tick && ppu_fifo[pixel_idx].scx_a)
	{
		pixel_idx += 1; // goto next pixel in fifo
		pixel_count -= 1; // "shift the pixel out"
		scx_cnt -= 1;
		if(!scx_cnt)
		{
			x_scroll_tick = false;
		}
		return (pixel_count > 8);
	}
	
	// assume bg winns
	bool sprite_priority = false;

	const auto bg = ppu_fifo[pixel_idx];

	const auto sp = fetcher_sprite.fifo[fetcher_sprite.read_idx];

	// if sprite has stuff to push
	// advance the fifo and figure out which one wins
	if(fetcher_sprite.len != 0)
	{
		// "push" the sprite object out
		fetcher_sprite.len -= 1;
		fetcher_sprite.read_idx = (fetcher_sprite.read_idx + 1) % fetcher_sprite.size;

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
	
	// shift out a pixel
	pixel_count -= 1;
	pixel_idx += 1; // goto next pixel in fifo
	if(++x_cord == 160)
	{
		// done drawing enter hblank
		hblank = true;
		return false;
	}
	return true;
}	
	





// todo proper scx and window timings
// as we current do not implement them at all 
// window should restart the fetcher when triggered 
// and take 6 cycles 

// fetcher operates at half of base clock
// therefore all cycle timings counts are doubled
// (as this just works out nicer than dividing the inputs by two)

// need to handle bugs with the window
void Ppu::tick_fetcher(int cycles) noexcept
{

	// advance the fetcher if we dont have a tile dump waiting
	// fetcher operates at half of base clock (4mhz)
	if(!tile_ready) // fetch the tile
	{
		// should fetch the number then low and then high byte
		// but we will ignore this fact for now

		// 1 cycle is tile num 
		// 2nd is lb of data 
		// 3rd is high byte of data 

		ppu_cyc += cycles; // further along 

		if(ppu_cyc >= 6) // takes 3 cycles to fetch 8 pixels
		{
			tile_fetch();
			tile_ready = true;
			// any over flow will be used to push the next tile
			// into the fifo (the 4th clock)
			ppu_cyc = 0;
		}	
	}
	
	// if we have room to dump into the fifo
	// and we are ready to do so, do it now 
	// at 0 dump at start at 8 pixels dump in higher half
	if(tile_ready && pixel_count <= 8)
	{
		// sanity check incase it fetches an extra
		// tile or two when pushing out the last set
		// of pixels
		if(tile_cord <= 160)
		{
			memcpy(&ppu_fifo[tile_cord],fetcher_tile,8 * sizeof(Pixel_Obj));
		}
		tile_cord += 8; // goto next tile fetch
		tile_ready = false;
		pixel_count += 8;
	}		
}	
	

void Ppu::draw_scanline(int cycles) noexcept 
{
	// get lcd control reg
	const int control = mem.io[IO_LCDC];
	
	
	tick_fetcher(cycles);
	
	// push out of fifo
	if(pixel_count > 8)
	{
		for(int i = 0; i < cycles; i++) // 1 pixel pushed per cycle
		{
			// ignore sprite timings for now
			if(is_set(control,1))
			{
				sprite_fetch();
			}
		
			// blit the pixel 
			// stop at hblank 
			// or if the fifo only has 8 pixels inside it
			if(!push_pixel()) 
			{ 
				return; 
			}
		}
	}
}

// fetch a single tile into the fifo

void Ppu::tile_fetch() noexcept
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
			fetcher_tile[i].colour_num = 0;
			fetcher_tile[i].source = pixel_source::tile;		
			fetcher_tile[i].scx_a = false;
		}
		return;
	}

	// where to draw the visual area and window
	const uint8_t scroll_y = mem.io[IO_SCY];
	const uint8_t scroll_x = mem.io[IO_SCX];
	const uint8_t window_y = mem.io[IO_WY];
	// window does not work nicely below 0x7 im not sure what the exact behavior we want here is...
	const uint8_t window_x = mem.io[IO_WX] & ~7; 
	const int scanline = current_line;
	

	// is the window enabled check in lcd control
	// and is the current scanline the window pos?
	// if we are using window 
	// it starts drawing at window_x 
	// so if we are less than it dont draw 
	const bool using_window = is_set(lcd_control,5) && (window_y <= scanline) 
		&&  (window_x <= 166) && (window_x <= tile_cord);
	
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
	
	for(int i = 0; i < 8; i++, color_bit += shift)
	{
		// combine data 2 and data 1 to get the color id for the pixel
		// in the tile
		int colour_num = val_bit(data2,color_bit) << 1;
		colour_num |= val_bit(data1,color_bit);
			
			
		// save our info to the fetcher
		// in dmg the pal number will be ignored
		fetcher_tile[i].colour_num = colour_num;
		fetcher_tile[i].cgb_pal = cgb_pal;
		// in cgb an priority bit is set it has priority over sprites
		// unless lcdc has the master overide enabled
		fetcher_tile[i].source = priority ? pixel_source::tile_cgbd : pixel_source::tile;		
		
		fetcher_tile[i].scx_a = !using_window;
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
	const uint8_t lcd_control = mem.io[IO_LCDC]; // get lcd control reg

	const int y_size = is_set(lcd_control,2) ? 16 : 8;
	
	const uint8_t scanline = current_line;

	
	// reset how many sprites we have as its a new scanline
	no_sprites = 0;
	for(int sprite = 0; sprite < 40; sprite++) // should fetch all these as soon as we leave oam search
	{
        const uint16_t addr = sprite*4;
		const uint8_t y_pos = mem.oam[addr & 0xff];
		if( scanline -(y_size - 16) < y_pos  && scanline + 16 >= y_pos )
		{
			// intercepts with the line
			objects_priority[no_sprites].index = addr; // save the index
			// and the x pos
			objects_priority[no_sprites].x_pos = mem.oam[(addr+1)&0xff]-8;
			if(++no_sprites == 10) { break; } // only draw a max of 10 sprites per line
		}
	}

	// if in dmg mode sort the array
	// lower x cord has highest priority
	// if at same x cord lowest oam draws first
	// once in the fifo writes will be blocked if they have a lower priority
	if(!cpu.get_cgb())
	{
		std::sort(&objects_priority[0],&objects_priority[no_sprites],
			[](const Obj &a, const Obj &b)
			{
				if(a.x_pos == b.x_pos)
				{
					return a.index < b.index;
				}

				return a.x_pos < b.x_pos;
			}
		);
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
		else if(is_set(sp.attr,7))
		{
			return false;
		}
	}

	return true;	
}


// returns if they have been rendered
// because we will delay if they have been
bool Ppu::sprite_fetch() noexcept 
{

    const bool is_cgb = cpu.get_cgb();
	
	const uint8_t lcd_control = mem.io[IO_LCDC]; // get lcd control reg

	const int y_size = is_set(lcd_control,2) ? 16 : 8;

	const int scanline = current_line;
	
	bool did_draw = false;
	
	for(int i = 0; i < no_sprites; i++)
	{
		// if wrap with the x posistion will cause it to wrap around
		// where its in range of the current sprite we still need to draw it
		// for thje pixels its in range 
		
		// say we have one at 255
		// it will draw 7 pixels starting from zero
		// so from 0-6
		// so if the xcord = 0 then the first 6 pixels must be mixed
		// but i have no clue how to actually do this under a fifo...
		
		
		// offset into the sprite we start drawing at 
		// 7 is the 0th pixel and is defualt for a 
		// sprite that we draw fully
		int pixel_start = 7; 

		uint8_t x_pos = objects_priority[i].x_pos;

		if(x_cord == 0 &&  x_pos + 7 > 255)
		{
			x_pos += 7;
			
			// this will cause it to draw at the correct offset into the sprite
			pixel_start = x_pos;
		}
		
		
		
		// if it does not start at the current x cord 
		// and does not overflow then we dont care
		else if(x_pos != x_cord)
		{
			continue;
		}
		
		
		
		
		// sprite takes 4 bytes in the sprite attributes table
		const uint8_t sprite_index = objects_priority[i].index;
		uint8_t y_pos = mem.oam[sprite_index];
		// lowest bit of tile index ignored for 16 pixel sprites
		const uint8_t sprite_location = y_size == 16? mem.oam[(sprite_index+2)] & ~1 : mem.oam[(sprite_index+2)];
		const uint8_t attributes = mem.oam[(sprite_index+3)];
		
		const bool y_flip = is_set(attributes,6);
		const bool x_flip = is_set(attributes,5);
		
		
		// does this sprite  intercept with the scanline
		if( scanline -(y_size - 16) < y_pos  && scanline + 16 >= y_pos )
		{
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

			// eaiser to read in from right to left as pixel 0
			// is bit 7 in the color data pixel 1 is bit 6 etc 
			for(int sprite_pixel = pixel_start; sprite_pixel >= 0; sprite_pixel--,colour_bit += shift)
			{

				// rest same as tiles
				const int colour_num = (val_bit(data2,colour_bit) << 1) 
					| val_bit(data1,colour_bit);

				// where we actually want to dump the pixel into the fifo
				uint8_t x_pix = 0 - sprite_pixel;
				x_pix += pixel_start;


				const auto fifo_idx = (x_pix + fetcher_sprite.read_idx) % fetcher_sprite.size;

				/* 
					lower objects_priority idx means that a fetcher object has a greater priority
					on dmg this will just work as priority is by x cordinate and therefore 
					the order sprites will be drawn in thus the fifo will fill up with
					higher priority sprites first and deny sprites that draw after it
					however on cgb they will draw out of priority order as its by oam idx
					so we need a manual check by here 
				*/

				// if a pixel is allready in the fifo
				if(x_pix < fetcher_sprite.len)
				{
					// current color is transparent and a pixel 
					// is allready there we lose
					if(colour_num == 0)
					{
						continue;
					}

					// pixel allready there has a higher priority
					// and color is not transparent we lose
					else if(fetcher_sprite.fifo[fifo_idx].priority < i && fetcher_sprite.fifo[fifo_idx].colour_num != 0)
					{
						continue;
					}
				}	


				fetcher_sprite.fifo[fifo_idx].colour_num = colour_num;
				fetcher_sprite.fifo[fifo_idx].source = is_set(attributes,4)? pixel_source::sprite_one : pixel_source::sprite_zero;
				
				// values just ignored in dmg
				fetcher_sprite.fifo[fifo_idx].cgb_pal = attributes & 0x7;	 
				fetcher_sprite.fifo[fifo_idx].attr = attributes;
				
				fetcher_sprite.fifo[fifo_idx].priority = i;	
			}
			// whatever length is longer is our new len ;)
			fetcher_sprite.len = (fetcher_sprite.len > pixel_start + 1)? fetcher_sprite.len : pixel_start + 1;
			did_draw = true;
		}
	}
	return did_draw;
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