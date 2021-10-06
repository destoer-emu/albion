#include <gb/gb.h>


namespace gameboy 
{

// -----------
// ppu viewer
// only used by the some frontends but its
// not code dependant on a frontend so its best placed here

std::vector<uint32_t> Ppu::render_bg(bool higher) noexcept
{

	std::vector<uint32_t> bg_map(256*256);

	const uint8_t lcd_control = mem.io[IO_LCDC]; // get lcd control reg
	int background_mem = higher ? 0x1c00 : 0x1800;

	const bool is_cgb = cpu.is_cgb;

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
					int color_num = is_set(data2,color_bit) << 1;
					color_num |= is_set(data1,color_bit);

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
	if(cpu.is_cgb)
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

	// dmg
	else
	{
		for(int p = 0; p < 3; p++)
		{
			for(int color_num = 0; color_num < 4; color_num++)
			{
				palette_bg[(p*4) + color_num] = dmg_pal[p][color_num];
			}
		}
	}
}

std::vector<uint32_t> Ppu::render_tiles() noexcept
{

	std::vector<uint32_t> tiles(384*8*8*2);

	int banks = cpu.is_cgb? 2 : 1;


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
						int color_num = is_set(data2,x) << 1;
						color_num |= is_set(data1,x);


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