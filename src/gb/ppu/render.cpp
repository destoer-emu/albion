#include <gb/gb.h>

namespace gameboy
{
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

				const bool is_tile = (pixel_ref.source <= pixel_source::tile);

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
		const auto tile_num = 256 + static_cast<int8_t>(mem.vram[0][tile_address]);
		tile_location = tile_num * 16;
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
		vram_bank = is_set(attr,3);
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
	
    // inverted because we go backwards
	unsigned int color_bit = x_flip? 7 : 0;
	
	const int shift = x_flip ? -1 : 1;

	// in cgb an priority bit is set it has priority over sprites
	// unless lcdc has the master overide enabled
	const auto source = priority ? pixel_source::tile_cgbd : pixel_source::tile;	

	for(int i = 7; i >= 0; i--, color_bit += shift)
	{
		// combine data 2 and data 1 to get the color id for the pixel
		// in the tile		
		buf[i].colour_num = (is_set(data2,color_bit) << 1) | is_set(data1,color_bit);

		// save our info to the fetcher in dmg the pal number will be ignored
		buf[i].cgb_pal = cgb_pal;
		buf[i].source = source;	
	}
}


uint32_t Ppu::get_dmg_color(int color_num, pixel_source source) const noexcept
{
	const auto source_idx = static_cast<int>(source);

	const int color_address = + source_idx + 0xff47;	
	const uint8_t palette = mem.io[color_address & 0xff];
	const int color_idx = (palette >> (color_num * 2)) & 3; 
	
	return dmg_pal[source_idx][color_idx];
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

	col = deset_bit(col,15);
	return col_lut[col];
}



// considering taking copies and threading this
void Ppu::render_scanline() noexcept
{

	// SGB: approximate mask_en only on scanline renderer
	switch(mask_en)
	{
		// standard
		case mask_mode::cancel: break;

		// TODO: this wont play nice on castlevania
		case mask_mode::freeze: return;
		// assume white
		case mask_mode::clear: 
		{
			for(u32 i = 0; i < screen.size(); i++)
			{
				screen[i] = 0xffffffff;
			}
			break;
		}

		case mask_mode::black:
		{
			for(u32 i = 0; i < screen.size(); i++)
			{
				screen[i] = 0xff000000;
			}
			break;
		}	
	}



	// is the window drawn on this line?
	const bool window_rendered = mem.io[IO_WX] <= 166 && 
		window_y_triggered && is_set(mem.io[IO_LCDC],5);

	window_x_triggered = window_rendered;
	
    const auto scx_offset = mem.io[IO_SCX] & 0x7;
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

    // is sprite drawing enabled?
	if(is_set(mem.io[IO_LCDC],1))
	{
		sprite_fetch(&scanline_fifo[scx_offset],false);
	}
	
    const uint32_t offset = (current_line*SCREEN_WIDTH);
    const bool is_cgb = cpu.get_cgb();
	for(int x = SCREEN_WIDTH-1; x >= 0; x--)
	{
		const auto pixel = scanline_fifo[x+scx_offset];
        
        const uint32_t full_color = is_cgb? get_cgb_color(pixel.colour_num, pixel.cgb_pal, pixel.source) :
            get_dmg_color(pixel.colour_num,pixel.source);

		screen[offset+x] = full_color;
	}

}


}