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
		for(u16 i = 0x134; i < 0x143; i++)
		{
			checksum += mem.read_mem(i);
		}

		printf("checumsum %x\n",checksum);
	*/
		memcpy(dmg_pal,dmg_colors,sizeof(dmg_pal));

	} 

	mask_en = mask_mode::cancel;

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
	if(!cpu.is_cgb)
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

	mem.unlock_vram();
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
	if(cpu.is_cgb)
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
	mem.unlock_vram();

}

void Ppu::update_graphics(u32 cycles) noexcept
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
				mem.lock_vram();
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
	// SGB: approximation
	if(mask_en != mask_mode::cancel)
	{
		return;
	}

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

u32 Ppu::calc_pixel_transfer_end() noexcept
{
	// does not handle the sprite delay
	u32 cycles = 252; // base
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

	const u32 full_color = cpu.is_cgb? get_cgb_color(pixel.colour_num, pixel.cgb_pal, pixel.source) :
		get_dmg_color(pixel.colour_num,pixel.source);

	screen[(current_line*SCREEN_WIDTH)+x_cord] = full_color;
	
	
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
	// advance the fetcher if we dont have a tile dump waiting
	// fetcher operates at half of base clock (4mhz)
	// note atm this only fetches tiles but on real hardware we fetch sprites and tiles :D
	if(!fetcher.ready) // fetch some data
	{
		// 1 cycle is tile num 
		// 2nd is lb of data 
		// 3rd is high byte of data 
		// 4th attempt to push 
		fetcher.cyc++; 
		
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
	

void Ppu::draw_scanline(u32 cycles) noexcept 
{
	// is sprite drawing enabled?
	const bool obj_enabled = is_set(mem.io[IO_LCDC],1);
	

	// advance the fetcher and the fifo
	for(u32 i = 0; i < cycles; i++) // 1 pixel pushed per cycle
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
		// we need to wait for the fetcher to be empty for this to happen 
		// on actual hardware
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
        const u16 addr = sprite*4;
		const uint8_t y_pos = mem.oam[addr & 0xff];
		if( scanline -(y_size - 16) < y_pos  && scanline + 16 >= y_pos )
		{
			// intercepts with the line
			objects[no_sprites].index = addr; // save the index
			// and the x pos
			objects[no_sprites].x_pos = mem.oam[(addr+1)&0xff];
			objects[no_sprites].attr = mem.oam[(addr+3)];

			// priority by oam index in cgb update here
			if(cpu.is_cgb)
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
	if(!cpu.is_cgb)
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
	const bool draw_over_everything = !is_set(mem.io[IO_LCDC],0) && cpu.is_cgb;

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


}
