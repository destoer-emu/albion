#include <gb/ppu.h>
#include <gb/cpu.h>
#include <gb/memory.h>

/* todo add tile & sprite caching? */
// also figure out why stat irq blocking no longer passes

namespace gameboy
{

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

}


// other stuff should proabably go in here i forgot to port over
void Ppu::init(Cpu *c,Memory *m) noexcept
{
    cpu = c;
    mem = m;
	screen.resize(SCREEN_WIDTH*SCREEN_HEIGHT);
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

	if(is_set(mem->io[IO_SPPI],7)) // increment on a write 
	{
		sp_pal_idx = (sp_pal_idx + 1) & 0x3f;
		mem->io[IO_SPPI] &= ~0x3f;
		mem->io[IO_SPPI] |= sp_pal_idx;
	}	
}

void Ppu::write_bgpd(uint8_t v) noexcept
{
	// cant be accessed during pixel transfer
	if(mode != ppu_mode::pixel_transfer)
	{
		bg_pal[bg_pal_idx] = v; 
	}

	if(is_set(mem->io[IO_BGPI],7)) // increment on a write 
	{
		bg_pal_idx = (bg_pal_idx + 1) & 0x3f;
		mem->io[IO_BGPI] &= ~0x3f;
		mem->io[IO_BGPI] |= bg_pal_idx;
	}	
}



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


void Ppu::write_stat()
{
	stat_update();
}

// mode change, write, or line change
// will trigger this
void Ppu::stat_update()
{
	if(!mem->is_lcd_enabled())
	{
		return;
	}

	// read stat and mask the mode
	uint8_t status = mem->io[IO_STAT];
	status &= ~0x3;

	// save our current signal state
	const bool signal_old = signal;

	// check interrupts
	switch(mode)
	{
		case ppu_mode::hblank: signal = is_set(status,3); break;
		case ppu_mode::vblank: signal = is_set(status,4); break;
		case ppu_mode::oam_search: signal = is_set(status,5); break;
		case ppu_mode::pixel_transfer: signal = false; break;
	}
	

	// check coincidence  (lyc == ly)
	// line 153 can be treated as zero 
	// see https://forums.nesdev.com/viewtopic.php?f=20&t=13727
	//if( lyc == ly || (lyc == 0 && ly == 153) )
	// if lyc is current line set coincidence bit else deset it
	status = (mem->io[IO_LYC] == current_line)? set_bit(status,2) : deset_bit(status,2);

	// still not 100% on lyc interacts with the signal edge
	if(is_set(status,6) && is_set(status,2))
	{
		signal = true;
	}	
	

	// if we have changed from 0 to 1 for signal(signal edge)
	// request a stat interrupt
	if(!signal_old && signal)
	{
		cpu->request_interrupt(1);	
	}
	
	// update our status reg
	mem->io[IO_STAT] = status | 128 | static_cast<int>(mode);
}

void Ppu::update_graphics(int cycles) noexcept
{
	//-----------------------
	// update the stat reg state
	// and trigger interrupts	
	
	// read out current stat reg and mask the mode
	uint8_t status = mem->io[IO_STAT];
	status &= ~0x3;


	// mode is one if lcd is disabled
	if(!mem->is_lcd_enabled()) // <-- should re enable on a delay?
	{

		scanline_counter = 0; // counter is reset?
		current_line = 0; // reset ly
		mem->io[IO_STAT] = status; 
		mode = ppu_mode::oam_search;
		signal = false; 
		return; // can exit if ppu is disabled nothing else to do
	}
	
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
					cpu->request_interrupt(0); // vblank interrupt
					
					// edge case oam stat interrupt is triggered here if enabled
					if(is_set(status,5))
					{
						if(signal == false)
						{
							cpu->request_interrupt(1);
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
					mem->io[IO_STAT] &= ~3;
					mem->io[IO_STAT] |= 2;
					current_line = 0;
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

				scx_cnt = (mem->io[IO_SCX] & 0x7);
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
				if(cpu->get_cgb())
				{
					mem->do_hdma();
				}
				stat_update();
			}

			break;
		}	
	}	
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
	
	
	const int col_num = ppu_fifo[pixel_idx].colour_num; // save the pixel we will shift
	const int scanline = current_line;
	
	if(!cpu->get_cgb())
	{
		int colour_address = static_cast<uint16_t>(ppu_fifo[pixel_idx].source) + 0xff47;	
		dmg_colors col = get_colour(col_num,colour_address); 

        // black is default
        uint32_t full_color = 0xff000000;

		switch(col)
		{
            case dmg_colors::black: /*full_color = 0x000000ff;*/ break;
			case dmg_colors::white: full_color = 0xffffffff; break;
			case dmg_colors::light_gray: full_color = 0xffcccccc;  break;
			case dmg_colors::dark_gray: full_color = 0xff777777; break;
		}

		screen[(scanline*SCREEN_WIDTH)+x_cord] = full_color;
	}
	
	else // gameboy color
	{
		// for now we will assume tile just for arugments sake 
		const int cgb_pal = ppu_fifo[pixel_idx].cgb_pal;
		// each  rgb value takes two bytes in the pallete for cgb
		const int offset = (cgb_pal*8) + (col_num * 2); 

		int col;
		if(ppu_fifo[pixel_idx].source == pixel_source::tile ||
			ppu_fifo[pixel_idx].source == pixel_source::tile_cgbd)
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

		screen[(scanline*SCREEN_WIDTH)+x_cord] = (full_color) | 0xff000000;
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
// and take 6 cycles (this is now done)

// and we should implement scx properly with the fetcher...
// ^ this needs researching and implementing

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

		if(ppu_cyc >= 3) // takes 3 cycles to fetch 8 pixels
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
	const int control = mem->io[IO_LCDC];
	
	// fetcher operates at half of base clock
	tick_fetcher(cycles/2);
	
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

    const bool is_cgb = cpu->get_cgb();

	// where to draw the visual area and window
	const uint8_t scroll_y = mem->io[IO_SCY];
	const uint8_t scroll_x = mem->io[IO_SCX];
	const uint8_t window_y = mem->io[IO_WY];
	const uint8_t window_x = mem->io[IO_WX] - 7; // 0,0 is at offest - 7 for window
	
	const int lcd_control = mem->io[IO_LCDC];
	

	const int scanline = current_line;
	
	// is the window enabled check in lcd control
	// and is the current scanline the window pos?
	const bool using_window = is_set(lcd_control,5) && (window_y <= scanline)
		&& (window_x < 166);

	
	// what kind of address are we using
	const bool unsig = is_set(lcd_control,4);
	
	
	// addresses are -0x8000 for vram accesses
	// as it starts at 0x8000 in the memory map
	
	// what tile data are we using
	const int tile_data = unsig ? 0 : 0x800; 
	
	
	// ypos is used to calc which of the 32 vertical tiles 
	// the current scanline is drawing	
	uint8_t y_pos = 0;
	

	int background_mem = 0;
	
	// which background mem?
	if(!using_window)
	{
		background_mem = is_set(lcd_control,3) ? 0x1c00 : 0x1800;
		y_pos = scroll_y + scanline;
	}
	
	else
	{
		// which window mem?
		background_mem = is_set(lcd_control,6) ? 0x1c00 : 0x1800;
		y_pos = scanline - window_y;
	}

	
	// which of the 8 vertical pixels of the scanline are we on
	const int tile_row = ((y_pos/8)*32);

	
	int x_pos = (tile_cord/8);
		
	if(!using_window) // <-- dont think this is correct
	{
		x_pos += (scroll_x/8);
	}
	
	// if we are using window 
	// it starts drawing at window_x 
	// so if we are less than it dont draw 
	else if(x_pos < window_x/8)
	{
		return;
	}

		
	x_pos &= 31;
	y_pos &= 7;
	// get the tile identity num it can be signed or unsigned
	// -0x8000 to account for the vram 
	const int tile_address = (background_mem + tile_row+x_pos);

	// deduce where this tile identifier is in memory
	int tile_location = tile_data;		

	int tile_num;
	// tile number is allways bank 0
	if(unsig)
	{
		tile_num = mem->vram[0][tile_address];
		tile_location += (tile_num *16);
	}
	
	else
	{
		tile_num = (int8_t)mem->vram[0][tile_address];
		tile_location += ((tile_num+128)*16);
	}

	int cgb_pal = -1;
	bool priority = false;
	bool x_flip = false;
	bool y_flip = false;
	int vram_bank = 0;


	if(is_cgb) // we are drawing in cgb mode 
	{
		// bg attributes allways in bank 1
		const uint8_t attr = mem->vram[1][tile_address];
		cgb_pal = attr & 0x7; // get the pal number
				
				
		// draw over sprites
		priority = is_set(attr,7);
				
		x_flip = is_set(attr,5);
		y_flip = is_set(attr,6);

		// decide what bank data is coming out of
		// allready one so dont check the other condition
		vram_bank = is_set(attr,3) ? 1 : 0;
	}
			
	// find the correct vertical line we are on of the
	// tile to get the tile data		
	// read the sprite backwards in y axis if y flipped
	const int line = y_flip? 14 - (y_pos*2) : (y_pos*2);
		
			
	const uint8_t data1 = mem->vram[vram_bank][(tile_location+line)];
	const uint8_t data2 = mem->vram[vram_bank][(tile_location+line+1)];
	
	// pixel 0 in the tile is bit 7 of data1 and data2
	// pixel 1 is bit 6 etc
	
	int color_bit = x_flip? 0 : 7;
	
	int shift = x_flip ? 1 : -1;
	
	for(int i = 0; i < 8; i++, color_bit += shift)
	{
		// combine data 2 and data 1 to get the color id for the pixel
		// in the tile
		int colour_num = val_bit(data2,color_bit) << 1;
		colour_num |= val_bit(data1,color_bit);
			
			
			
		// save the color_num to the current pos int the tile fifo		
		if(!is_cgb)
		{
			fetcher_tile[i].colour_num = colour_num;
			fetcher_tile[i].source = pixel_source::tile;		
		}
			
		else // cgb save the pallete value... 
		{
			fetcher_tile[i].colour_num = colour_num;
			fetcher_tile[i].cgb_pal = cgb_pal;
			// in cgb an priority bit is set it has priority over sprites
			// unless lcdc has the master overide enabled
			fetcher_tile[i].source = priority ? pixel_source::tile_cgbd : pixel_source::tile;		
		}
		fetcher_tile[i].scx_a = !using_window;
	}
}

dmg_colors Ppu::get_colour(uint8_t colour_num, uint16_t address) noexcept
{
	const uint8_t palette = mem->io[address & 0xff];
	int hi = 0;
	int lo = 0;
	
	switch(colour_num)
	{
		case 0: hi = 1; lo = 0; break;
		case 1: hi = 3; lo = 2; break;
		case 2: hi = 5; lo = 4; break;
		case 3: hi = 7; lo = 6; break;
	}
	
	// use the palette to get the color
	int colour = 0;
	colour = val_bit(palette,hi) << 1;
	colour |= val_bit(palette,lo);
	

	constexpr dmg_colors colors[] = {dmg_colors::white,dmg_colors::light_gray,
        dmg_colors::dark_gray,dmg_colors::black};

	return colors[colour];
}




bool cmpfunc(const Obj &a, const Obj &b) noexcept
{
	// sort by the oam index
	if(a.x_pos == b.x_pos)
	{
		return (a.index > b.index);
	}

	// sort by the x posistion
	else
	{
		return (a.x_pos < b.x_pos);
	}
}



// read the up to 10 sprites for the scanline
// called when when enter pixel transfer
void Ppu::read_sprites() noexcept
{
	const uint8_t lcd_control = mem->io[IO_LCDC]; // get lcd control reg

	const int y_size = is_set(lcd_control,2) ? 16 : 8;
	
	const uint8_t scanline = current_line;


	int x = 0;
	for(int sprite = 0; sprite < 40; sprite++) // should fetch all these as soon as we leave oam search
	{
        const uint16_t addr = sprite*4;
		const uint8_t y_pos = mem->oam[addr & 0xff];
		if( scanline -(y_size - 16) < y_pos  && scanline + 16 >= y_pos )
		{
			// intercepts with the line
			objects_priority[x].index = addr; // save the index
			// and the x pos
			objects_priority[x].x_pos = mem->oam[(addr+1)&0xff]-8;
			if(++x == 10) { break; } // only draw a max of 10 sprites per line
		}
	}

	
	// sort the array
	// if x cords are same use oam as priority lower indexes draw last
	// else use the x cordinate again lower indexes draw last
	// this means they will draw on top of other sprites
	std::sort(&objects_priority[0],&objects_priority[x],cmpfunc);	
	
	no_sprites = x; // save how many sprites we have	
}



// returns if they have been rendered
// because we will delay if they have been
bool Ppu::sprite_fetch() noexcept 
{

    const bool is_cgb = cpu->get_cgb();
	
	const uint8_t lcd_control = mem->io[IO_LCDC]; // get lcd control reg

	// in cgb if lcdc bit 0 is deset sprites draw over anything
	const bool draw_over_everything = (!is_set(lcd_control,0) && is_cgb);
	
	const int y_size = is_set(lcd_control,2) ? 16 : 8;

	const int scanline = current_line;
	
	bool did_draw = false;
	
	for(int i = 0; i < no_sprites; i++)
	{
		int vram_bank = 0;
		uint8_t x_pos = objects_priority[i].x_pos;
		
		
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
		uint8_t y_pos = mem->oam[sprite_index];
		const uint8_t sprite_location = mem->oam[(sprite_index+2)];
		const uint8_t attributes = mem->oam[(sprite_index+3)];
		
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
			
			line *= 2; // same as for tiles
			uint16_t data_address = ((sprite_location * 16 )) + line; // in realitly this is offset into vram at 0x8000
			if(is_set(attributes,3) && is_cgb) // if in cgb and attr has bit 3 set 
			{
				vram_bank = 1; // sprite data is out of vram bank 1
			}
				

			const uint8_t data1 = mem->vram[vram_bank][data_address];
			const uint8_t data2 = mem->vram[vram_bank][(data_address+1)];
			
			// eaiser to read in from right to left as pixel 0
			// is bit 7 in the color data pixel 1 is bit 6 etc 
			Pixel_Obj *fifo = &ppu_fifo[pixel_idx];
			for(int sprite_pixel = pixel_start; sprite_pixel >= 0; sprite_pixel--)
			{
				int colour_bit = sprite_pixel;
				// red backwards for x axis
				if(x_flip)
				{
					colour_bit = 7 - colour_bit;
				}
				
				// rest same as tiles
				int colour_num = val_bit(data2,colour_bit);
				colour_num <<= 1;
				colour_num |= val_bit(data1,colour_bit);

				
				
				
				// colour_num needs to be written out ot sprite_fetcher
				// however in our array that will be shifted to the screen
				// we need to say where it is from eg tile or sprite
				// so that we actually know where to read the color from 
				// so we need to implement an array with the color id and a bool 
				// so that it knows where the colour values have actually come from 
				
				// dont display pixels with color id zero
				// the color itself dosent matter we only care about the id
				
				uint8_t x_pix = 0 - sprite_pixel;
				x_pix += pixel_start;

				
				// transparent sprite so the tile wins
				if(colour_num == 0)
				{
					continue;
				}
				
				
				// test if its hidden behind the background layer
				// white is transparent even if the flag is set
				if(is_set(attributes,7))
				{
					if(fifo[x_pix].colour_num != 0)
					{
						// need to test for the overide in lcdc on cgb
						if(!draw_over_everything)
						{
							continue;
						}
					}	
				}
				
				
				pixel_source source = is_set(attributes,4)? pixel_source::sprite_one : pixel_source::sprite_zero;
			

				// if the current posisiton is the fifo is not a sprite
				// then this current pixel wins 
				
				
				// in cgb if tile has priority set in its attributes it will draw over it 
				// unless overidded by lcdc
				
				// if this is not set it can only draw if the tile does not have its priority 
				// bit set in cgb mode
				
				
				if(fifo[x_pix].source != pixel_source::tile_cgbd || draw_over_everything)
				{
					fifo[x_pix].colour_num = colour_num;
					fifo[x_pix].source = source;
					//if(is_cgb) probably faster to just write the value anyways
					//{
						fifo[x_pix].cgb_pal = attributes & 0x7;
					//}
					fifo[x_pix].scx_a = false;
				}	
			}
			did_draw = true;
		}
	}

	return did_draw;
}

}