#include <gb/memory.h>
#include <gb/ppu.h>
#include <gb/cpu.h>
#include <gb/apu.h>
#include <destoer-emu/lib.h>
#include <destoer-emu/debug.h>

namespace gameboy
{


bool Memory::is_lcd_enabled() const noexcept
{
	return ( is_set(io[IO_LCDC],7) );	
}

void Memory::save_cart_ram()
{
	std::string filename = rom_info.filename;
	if(filename == "")
	{
		return;
	}

	std::string save_name = filename;

	size_t ext_idx = filename.find_last_of("."); 
	if(ext_idx != std::string::npos)
	{
		save_name = filename.substr(0, ext_idx); 	
	}

	save_name += ".sav";

    auto fp = std::fstream(save_name, std::ios::out | std::ios::binary);
	for(auto &x : cart_ram_banks)
	{
    	fp.write(reinterpret_cast<char*>(x.data()), x.size());
	}
    fp.close();
}

void Memory::load_cart_ram()
{
	std::string filename = rom_info.filename;
	if(filename == "")
	{
		return;
	}

	std::string save_name = filename;

	size_t ext_idx = filename.find_last_of("."); 
	if(ext_idx != std::string::npos)
	{
		save_name = filename.substr(0, ext_idx); 	
	}

	save_name += ".sav";

    auto fp = std::fstream(save_name, std::ios::in | std::ios::binary);
	for(auto &x : cart_ram_banks)
	{
    	fp.read(reinterpret_cast<char*>(x.data()), x.size());
	}
    fp.close();	
}

bool Memory::rom_cgb_enabled() const noexcept
{
	switch(rom[0x143])
	{
		case 0x80: return true; // add options to run cgb in dmg1
		case 0xc0: return true;
		default: return false;
	}
}

void Memory::init(Cpu *c,Ppu *p,Debug *d,Apu *a,std::string rom_name, bool with_rom, bool use_bios)
{
    cpu = c; // init our cpu pointer
    ppu = p;
	apu = a;
	debug = d;

	if(with_rom)
	{
		read_file(rom_name,rom); // read our rom in


		// propagate an error back with an exception later for now we just bail
		if(rom.size() < 0x4000)
		{
			throw std::runtime_error("rom is too small!");
		}
	}

	else
	{
		rom.resize(0x4000);
	}

	if(use_bios)
	{
		std::string bios_file = rom_cgb_enabled() ? "gbc_bios.bin" : "dmg_bios.bin";
		try
		{
			read_file(bios_file,bios);
		}

		catch(std::exception &ex)
		{
			UNUSED(ex);
			throw std::runtime_error(fmt::format("could not load bios file: {}!",bios_file));
		}
	}

    // reserve our underlying memory
    vram.resize(0x2);
    for(auto &x: vram)
    {
        x.resize(0x2000); 
		std::fill(x.begin(),x.end(),0);
    }


    wram.resize(0x1000);
	std::fill(wram.begin(),wram.end(),0); 
    oam.resize(0xa0);
	std::fill(oam.begin(),oam.end(),0);
    io.resize(0x100);
	std::fill(io.begin(),io.end(),0);


    // pull out our rom info
    rom_info.init(rom,rom_name);


    cart_ram_banks.resize(rom_info.no_ram_banks);
    for(auto &x: cart_ram_banks)
    {
        x.resize(0x2000);
		std::fill(x.begin(),x.end(),0);
    }

	load_cart_ram();


    cgb_wram_bank.resize(7);
    for(auto &x: cgb_wram_bank)
    {
        x.resize(0x1000);
		std::fill(x.begin(),x.end(),0);
    }



    // init our function table

    // read_mem
	memory_table[0x0].read_memf = &Memory::read_bank_zero;
	memory_table[0x1].read_memf = &Memory::read_bank_zero;
	memory_table[0x2].read_memf = &Memory::read_bank_zero;
	memory_table[0x3].read_memf = &Memory::read_bank_zero;
	memory_table[0x4].read_memf = &Memory::read_rom_bank;
	memory_table[0x5].read_memf = &Memory::read_rom_bank;
	memory_table[0x6].read_memf = &Memory::read_rom_bank;
	memory_table[0x7].read_memf = &Memory::read_rom_bank;
	memory_table[0x8].read_memf = &Memory::read_vram;
	memory_table[0x9].read_memf = &Memory::read_vram;
	memory_table[0xa].read_memf = &Memory::read_cart_ram;
	memory_table[0xb].read_memf = &Memory::read_cart_ram;
	memory_table[0xc].read_memf = &Memory::read_wram_low;
	memory_table[0xd].read_memf = &Memory::read_wram_high;
	memory_table[0xe].read_memf = &Memory::read_wram_low;
	memory_table[0xf].read_memf = &Memory::read_hram;


    // write mem
	memory_table[0x8].write_memf = &Memory::write_vram;
	memory_table[0x9].write_memf = &Memory::write_vram;
	memory_table[0xa].write_memf = &Memory::write_cart_ram;
	memory_table[0xb].write_memf = &Memory::write_cart_ram;
	memory_table[0xc].write_memf = &Memory::write_wram_low;
	memory_table[0xd].write_memf = &Memory::write_wram_high;
	memory_table[0xe].write_memf = &Memory::write_wram_low;
	memory_table[0xf].write_memf = &Memory::write_hram;

	switch(rom_info.type)
	{
		// mbc1 rom
		case rom_type::mbc1:
		{
			
			memory_table[0x0].read_memf = &Memory::read_rom_lower_mbc1;
			memory_table[0x1].read_memf = &Memory::read_rom_lower_mbc1;
			memory_table[0x2].read_memf = &Memory::read_rom_lower_mbc1;
			memory_table[0x3].read_memf = &Memory::read_rom_lower_mbc1;

			memory_table[0x0].write_memf = &Memory::ram_bank_enable;
			memory_table[0x1].write_memf = &Memory::ram_bank_enable;
			memory_table[0x2].write_memf = &Memory::change_lo_rom_bank_mbc1;
			memory_table[0x3].write_memf = &Memory::change_lo_rom_bank_mbc1;
			memory_table[0x4].write_memf = &Memory::mbc1_banking_change;
			memory_table[0x5].write_memf = &Memory::mbc1_banking_change;
			memory_table[0x6].write_memf = &Memory::change_mode_mbc1;
			memory_table[0x7].write_memf = &Memory::change_mode_mbc1;
			break;        
		}


		// mbc2 rom
		case rom_type::mbc2:
		{
			memory_table[0x0].write_memf = &Memory::lower_bank_write_mbc2;
			memory_table[0x1].write_memf = &Memory::lower_bank_write_mbc2;
			memory_table[0x2].write_memf = &Memory::lower_bank_write_mbc2;
			memory_table[0x3].write_memf = &Memory::lower_bank_write_mbc2;
			
			// no ram bank changing
			memory_table[0x4].write_memf = &Memory::banking_unused;
			memory_table[0x5].write_memf = &Memory::banking_unused;
			memory_table[0x6].write_memf = &Memory::banking_unused;
			memory_table[0x7].write_memf = &Memory::banking_unused;


			// mbc2 memory needs special handlers for its built in sram
			memory_table[0xa].write_memf = &Memory::write_cart_ram_mbc2;
			memory_table[0xb].write_memf = &Memory::write_cart_ram_mbc2;
			memory_table[0xa].read_memf = &Memory::read_cart_ram_mbc2;
			memory_table[0xb].read_memf = &Memory::read_cart_ram_mbc2;
			break;
		}

		// mbc3 rom
		case rom_type::mbc3:
		{
			memory_table[0x0].write_memf = &Memory::ram_bank_enable_mbc5;
			memory_table[0x1].write_memf = &Memory::ram_bank_enable_mbc5;
			memory_table[0x2].write_memf = &Memory::change_rom_bank_mbc3;
			memory_table[0x3].write_memf = &Memory::change_rom_bank_mbc3;
			memory_table[0x4].write_memf = &Memory::mbc3_ram_bank_change;
			memory_table[0x5].write_memf = &Memory::mbc3_ram_bank_change;
			memory_table[0x6].write_memf = &Memory::banking_unused;
			memory_table[0x7].write_memf = &Memory::banking_unused;
			break;
		}

		// mbc5 rom
		case rom_type::mbc5:
		{
			memory_table[0x0].write_memf = &Memory::ram_bank_enable;
			memory_table[0x1].write_memf = &Memory::ram_bank_enable;
			memory_table[0x2].write_memf = &Memory::change_lo_rom_bank_mbc5;
			memory_table[0x3].write_memf = &Memory::change_hi_rom_bank_mbc5;
			memory_table[0x4].write_memf = &Memory::mbc5_ram_bank_change;
			memory_table[0x5].write_memf = &Memory::mbc5_ram_bank_change;
			memory_table[0x6].write_memf = &Memory::banking_unused;
			memory_table[0x7].write_memf = &Memory::banking_unused;		
			break;
		}

		// rom only
		case rom_type::rom_only:
		{
			for(int i = 0; i < 8; i++)
			{
				memory_table[i].write_memf = &Memory::banking_unused;
			}
			break;
		}
		
		default:
		{
			throw std::runtime_error("unknown banking type!");
		}
	}

	if(!use_bios)
	{
		// init io
		io[0x10] = 0x80;
		io[0x11] = 0xBF;	
		io[0x12] = 0xF3;
		io[0x14] = 0xBF;
		io[0x16] = 0x3F;
		io[0x19] = 0xBF;
		io[0x1A] = 0x7F;
		io[0x1B] = 0xFF;
		io[0x1C] = 0x9F;
		io[0x1E] = 0xBF;
		io[0x20] = 0xFF;
		io[0x23] = 0xBF;
		io[0x24] = 0x77;
		io[0x25] = 0xF3;
		io[0x26] = 0xF1;
		io[0x40] = 0x91;
		io[0x47] = 0xFC;
		io[0x48] = 0xFF;
		io[0x49] = 0xFF;
	}
		
		
	
	// init unused hwio
	io[0x03] = 0xff;
	io[0x08] = 0xff;
	io[0x09] = 0xff;
	io[0x0a] = 0xff;
	io[0x0b] = 0xff;
	io[0x0c] = 0xff;
	io[0x0d] = 0xff;
	io[0x0e] = 0xff;
	io[0x15] = 0xff;
	io[0x1f] = 0xff;
	io[0x27] = 0xff;
	io[0x28] = 0xff;
	io[0x29] = 0xff;
	io[0x20] = 0xff;
	

    // banking vars
    enable_ram = false; // is ram banking enabled
    cart_ram_bank = (rom_info.no_ram_banks > 0) ? 0 : CART_RAM_BANK_INVALID;
	cart_rom_bank = 1; // currently selected rom bank
	rom_banking = true; // is rom banking enabled
	mbc1_bank2 = 0;

    // oam dma
	oam_dma_active = false; // indicate a dma is active and to lock memory
	oam_dma_address = 0; // the source address
	oam_dma_index = 0; // how far along the dma transfer we are    

    cgb_wram_bank_idx = 0;  // default zero
    vram_bank = 0; // what cgb vram bank are we in?

    hdma_len = 0; // length to transfer on a  gdma
	hdma_len_ticked = 0; // how many total dma transfers we have done
	dma_src = 0;
	dma_dst = 0;
	hdma_active = false;

	test_result = emu_test::running;
	gekkio_pass_count = 0;
	gekkio_fail_count = 0;
}



uint8_t Memory::read_bios(uint16_t addr) const noexcept
{
	if(cpu->get_cgb())
	{
		if(addr < bios.size() && (addr < 0x100  || addr >= 0x200))
		{
			return bios[addr];
		}


		else
		{
			return rom[addr];
		}
	}

	else
	{
		if(addr < bios.size() && addr < 0x100)
		{
			return bios[addr];
		}

		else
		{
			return rom[addr];
		}		
	}
}

void Memory::bios_enable() noexcept
{
	memory_table[0x0].read_memf = &Memory::read_bios;
	memory_table[0x1].read_memf = &Memory::read_bios;
	memory_table[0x2].read_memf = &Memory::read_bios;
	memory_table[0x3].read_memf = &Memory::read_bios;
}

void Memory::bios_disable() noexcept
{
	memory_table[0x0].read_memf = &Memory::read_bank_zero;
	memory_table[0x1].read_memf = &Memory::read_bank_zero;
	memory_table[0x2].read_memf = &Memory::read_bank_zero;
	memory_table[0x3].read_memf = &Memory::read_bank_zero;
}

void Memory::raw_write(uint16_t addr, uint8_t v) noexcept
{
	switch((addr & 0xf000) >> 12)
	{
		// bank zero
		case 0:  case 1: case 2: case 3:
		{
			rom[addr] = v;
			break;
		} 

		// rom (banked)
		case 4: case 5: case 6: case 7:
		{
			rom[cart_rom_bank*0x4000+(addr&0x3fff)] = v;
			break;
		}

		// vram
		case 8: case 9: 
		{
			vram[vram_bank][addr & 0x1fff] = v;
			break;
		}

		// cart ram
		case 0xa: case 0xb:
		{
			if(cart_ram_bank != CART_RAM_BANK_INVALID)
			{
				cart_ram_banks[cart_ram_bank][addr & 0x1fff] = v;
			}
			break;
		}

		// wram low
		case 0xc:
		{
			wram[addr&0xfff] = v;
			break;
		}

		// wram high
		case 0xd:
		{
			cgb_wram_bank[cgb_wram_bank_idx][addr&0xfff] = v;
			break;
		}

		// echo ram
		case 0xe: 
		{
			wram[addr&0xfff] = v;
			break;
		}

		case 0xf: // hram
		{
			// io regs
			if(addr >= 0xff00)
			{
				io[addr & 0xff] = v;
			}

			// high wram mirror
			else if(addr >= 0xf000 && addr <= 0xfdff)
			{
				cgb_wram_bank[cgb_wram_bank_idx][addr&0xfff] = v;
			}

			// oam is accesible during mode 0-1
			else if(addr >= 0xfe00 && addr <= 0xfe9f)
			{
				oam[addr & 0x9f] = v;
			}


			else // restricted
			{
			}
			break;
		}
	}
}


uint8_t Memory::raw_read(uint16_t addr) noexcept
{
	switch((addr & 0xf000) >> 12)
	{
		// bank zero
		case 0: case 1: case 2: case 3:
		{
			return rom[addr];
			break;
		} 

		// rom (banked)
		case 4: case 5: case 6: case 7:
		{
			return rom[cart_rom_bank*0x4000+(addr&0x3fff)];
			break;
		}

		// vram
		case 8: case 9: 
		{
			return vram[vram_bank][addr & 0x1fff];
			break;
		}

		// cart ram
		case 0xa: case 0xb:
		{
			if(cart_ram_bank != CART_RAM_BANK_INVALID)
			{
				return cart_ram_banks[cart_ram_bank][addr & 0x1fff];
			}

			else
			{
				return 0xff;
			}
			break;
		}

		// wram low
		case 0xc:
		{
			return wram[addr&0xfff];
			break;
		}

		// wram high
		case 0xd:
		{
			return cgb_wram_bank[cgb_wram_bank_idx][addr&0xfff];
			break;
		}

		// echo ram
		case 0xe: 
		{
			return wram[addr&0xfff];
			break;
		}

		case 0xf: // hram
		{
			// io regs
			if(addr >= 0xff00)
			{
				return read_io(addr);
			}

			// high wram mirror
			else if(addr >= 0xf000 && addr <= 0xfdff)
			{
				return cgb_wram_bank[cgb_wram_bank_idx][addr&0xfff];
			}

			// oam is accesible during mode 0-1
			else if(addr >= 0xfe00 && addr <= 0xfe9f)
			{
				return oam[addr & 0x9f];
			}


			else // restricted
			{
			}
			break;
		}
	}
	return 0xff; // should not be reached
}


// public access functions
uint8_t Memory::read_mem(uint16_t addr) noexcept
{
#ifdef DEBUG
	const uint8_t value = std::invoke(memory_table[(addr & 0xf000) >> 12].read_memf,this,addr);
	if(debug->breakpoint_hit(addr,value,break_type::read))
	{
		// halt until told otherwhise :)
		write_log("[DEBUG] read breakpoint hit ({:x}:{:x})",addr,value);
		debug->halt();
	}
	return value;

#else 
    return std::invoke(memory_table[(addr & 0xf000) >> 12].read_memf,this,addr);
#endif
}

void Memory::write_mem(uint16_t addr, uint8_t v) noexcept
{
#ifdef DEBUG
	if(debug->breakpoint_hit(addr,v,break_type::write))
	{
		// halt until told otherwhise :)
		write_log("[DEBUG] write breakpoint hit ({:x}:{:})",addr,v);
		debug->halt();
	}
#endif
    return std::invoke(memory_table[(addr & 0xf000) >> 12].write_memf,this,addr,v);    
}


uint16_t Memory::read_word(uint16_t addr) noexcept
{
    return read_mem(addr) | (read_mem(addr+1) << 8);
}


void Memory::write_word(uint16_t addr, uint16_t v) noexcept
{
    write_mem(addr+1,(v&0xff00)>>8);
    write_mem(addr,(v&0x00ff));
}


// memory accesses (timed)
uint8_t Memory::read_memt(uint16_t addr) noexcept
{
    uint8_t v = read_mem(addr);
    cpu->cycle_tick(1); // tick for the memory access 
    return v;
}

void Memory::write_memt(uint16_t addr, uint8_t v) noexcept
{
    write_mem(addr,v);
    cpu->cycle_tick(1); // tick for the memory access    
}


uint16_t Memory::read_wordt(uint16_t addr) noexcept
{
    return read_memt(addr) | (read_memt(addr+1) << 8);
}


void Memory::write_wordt(uint16_t addr, uint16_t v) noexcept
{
    write_memt(addr+1,(v&0xff00)>>8);
    write_memt(addr,(v&0x00ff));
}


// read mem underyling
// object attribute map 0xfe00 - 0xfe9f
uint8_t Memory::read_oam(uint16_t addr) const noexcept
{
    // cant access oam during a dma
    if(oam_dma_active)
    {
        return 0xff;
    }

    // if not in vblank or hblank cant access it
    if(ppu->mode != ppu_mode::hblank && ppu->mode != ppu_mode::vblank)
    {
        return 0xff;
    }

    return oam[addr & 0xff];
}

// video ram 0x8000 - 0xa000
uint8_t Memory::read_vram(uint16_t addr) const noexcept
{
    // vram is used in pixel transfer cannot access
    if(ppu->mode != ppu_mode::pixel_transfer)
    {
        return vram[vram_bank][addr & 0x1fff];
    }

    else
    {
        return 0xff;
    }
}

// cart  memory 0xa000 - 0xc000
uint8_t Memory::read_cart_ram(uint16_t addr) const noexcept
{ 
    if(enable_ram && cart_ram_bank != CART_RAM_BANK_INVALID)
    {
        return cart_ram_banks[cart_ram_bank][addr & 0x1fff];
    }

    else
    {
        return 0xff;
    }
}

// only bottom 4 bits are readable
// cart  memory 0xa000 - 0xc000
uint8_t Memory::read_cart_ram_mbc2(uint16_t addr) const noexcept
{
    if(enable_ram) // fixed for 512by4 bits
    {
        return cart_ram_banks[0][addr & 0x1ff];
    }

	else
	{
		return 0xff;
	}
}


// 0xff00 io regs (has side affects)
uint8_t Memory::read_io(uint16_t addr) const noexcept
{
    switch(addr & 0xff)
    {
        // joypad control reg <-- used for sgb command packets too
		case IO_JOYPAD:
		{		
			// read from mem
			const uint8_t req = io[IO_JOYPAD];
			// we want to test if bit 5 and 4 are set 
			// so we can determine what the game is interested
			// in reading
						
				
			// read out dpad 
			if(!is_set(req,4))
			{
				return ( (req & 0xf0) | (cpu->joypad_state & 0x0f) );
			}
			// read out a b sel start 
			else if(!is_set(req,5))
			{
				return ( (req & 0xf0) | ((cpu->joypad_state >> 4) & 0xf ) );
			}		
				
			return 0xff; // return all unset
		}	


        case IO_LY:
        {
            return ppu->current_line;
        }

		case IO_DIV:
		{
			// div register is upper 8 bits of the internal timer
			return (cpu->internal_timer & 0xff00) >> 8;
		}

		// sound regs
		// nr 11 only 7 and 6 readable
		case IO_NR11:
		{
			return (io[IO_NR11] & (128 + 64)) | (0xff-(128+64));
		}
			
		// write only
		case IO_NR13:
		{
			return 0xff;
		}
			
		// nr 14 only 6 is readable
		case IO_NR14:
		{
			return (io[IO_NR14] & (64)) | (0xff-64);
		}
			
		// nr 21 only bits 6-7 are r 
		case IO_NR21:
		{
			return (io[IO_NR21] & (128 + 64)) | (0xff-(128+64));		
		}
			
		// nr 23 write only
		case IO_NR23:
		{
			return 0xff;
		}
			
		// nr 24 only bit 6 can be read 
		case IO_NR24:
		{
			return (io[IO_NR24] & (64)) | (0xff-64);	
		}
			
		// nr 30 only bit 7
		case IO_NR30:
		{
			return (io[IO_NR30] & (128)) | (0xff-128);	
		}
			
		// nr 31 <-- unsure
		case IO_NR31:
		{
			return 0xff;
		}
			
		// nr 32 6-5 r
		case IO_NR32:
		{
			return (io[IO_NR32] & (64 + 32)) | (0xff-(64+32));
		}
			
		// nr33 write only
		case IO_NR33:
		{
			return 0xff;
		}
			
		// nr 34 6 r
		case IO_NR34:
		{
			return (io[IO_NR34] & (64)) | (0xff-64);
		}
			
		// nr 41
		case IO_NR41:
		{
			return io[IO_NR41] | 192;
		}

		// nr 44 bit 6
		case IO_NR44:
		{
			return (io[IO_NR44] & (64)) | (0xff-64);		
		}		


		// unused
		case 0x2a: case 0x2b: case 0x2c: 
		case 0x2d: case 0x2e: case 0x2f:
		{
			return 0xff;
		}

		// wave table
		case 0x30: case 0x31: case 0x32: case 0x33:
		case 0x34: case 0x35: case 0x36: case 0x37:
		case 0x38: case 0x39: case 0x3a: case 0x3b:
		case 0x3c: case 0x3d: case 0x3e: case 0x3f:
		{
			// if wave is on write to current byte <-- finish accuracy later
			if(apu->chan_enabled(2))
			{
				return io[0x30 + (apu->c3.get_duty_idx() / 2)];
			}
			
			else // if its off allow "free reign" over it
			{
				return io[addr & 0xff];	
			}
		}



		// CGB
		
		
		
		// cgb vram bank unused on dmg
		case IO_VBANK:
		{
			return io[IO_VBANK];
		}

		
		case IO_BGPD: 
		{
			return ppu->get_bgpd();
		}
		
		case IO_SPPD:
		{
			return ppu->get_sppd();
		}

		
		case IO_HDMA5: // verify
		{
			if(!hdma_active)
			{
				return 0xff;
			}
			
			// return the lenght tick left
			else 
			{
				return (hdma_len - 1);
			}
		}




        default: 
        {
            return io[addr & 0xff];
        }
    }
}

uint8_t Memory::read_iot(uint16_t addr) noexcept
{
    uint8_t v = read_io(addr);
    cpu->cycle_tick(1); // tick for mem access
    return v;
}

// for now we will just return the rom
// 0x4000 - 0x8000 return current rom bank
uint8_t Memory::read_rom_bank(uint16_t addr) const noexcept
{  
	return rom[cart_rom_bank * 0x4000 + (addr & 0x3fff)];
}



uint8_t Memory::read_rom_lower_mbc1(uint16_t addr) const noexcept
{
	if(rom_banking) // in rom banking its allways bank zero
	{
		return rom[addr & 0x3fff];
	}

	// if not the bank2 reg is used (shifted right by 5 bits)
	// which we do anyways because its the only way its ever used
	else // TODO optimise this bank read
	{
		int bank = mbc1_bank2 << 5;
		if(bank >= rom_info.no_rom_banks) 
		{
			bank %= rom_info.no_rom_banks;
		}
		return rom[(bank * 0x4000) +  (addr & 0x3fff)];
	}
}

//0x0000 - 0x4000 return rom bank zero
uint8_t Memory::read_bank_zero(uint16_t addr) const noexcept
{
    return rom[addr];
}

// 0xc000 && 0xe000 (echo ram)
// return wram non banked
uint8_t Memory::read_wram_low(uint16_t addr) const noexcept
{
    return wram[addr&0xfff];
}

// 0xd000
// return banked wram this is fixed on dmg
uint8_t Memory::read_wram_high(uint16_t addr) const noexcept
{
    return cgb_wram_bank[cgb_wram_bank_idx][addr&0xfff];
}

// 0xf000 various
uint8_t Memory::read_hram(uint16_t addr) const noexcept
{
    // io regs
    if(addr >= 0xff00)
    {
        return read_io(addr);
    }

    // high wram mirror
    else if(addr >= 0xf000 && addr <= 0xfdff)
    {
        return read_wram_high(addr);
    }

	// oam is accesible during mode 0-1
	else if(addr >= 0xfe00 && addr <= 0xfe9f)
	{
		return read_oam(addr);
	}


    else // restricted
    {
        return 0xff;
    }
}


// write mem underlying
// object attribute map 0xfe00 - 0xfea0
void Memory::write_oam(uint16_t addr,uint8_t v) noexcept
{

    if(oam_dma_active)
    {
        return;
    }

    // if not in vblank or hblank cant access it
    if(ppu->mode != ppu_mode::hblank && ppu->mode != ppu_mode::vblank)
    {
        return;
    }

    oam[addr & 0xff] = v;    
}

//video ram 0x8000 - 0xa000
void Memory::write_vram(uint16_t addr,uint8_t v) noexcept
{
    // vram is used in pixel transfer cannot access
    if(ppu->mode != ppu_mode::pixel_transfer)
    {
        vram[vram_bank][addr & 0x1fff] = v;
    }
}


void Memory::do_dma(uint8_t v) noexcept
{
	io[IO_DMA] = v; // write to the dma reg
	uint16_t dma_address = v << 8;
	// transfer is from 0xfe00 to 0xfea0
			
	/*// must be page aligned revisit later
	if(dma_address % 0x100) return;		
	*/

	// source must be below 0xe000
	// tick immediatly but keep the timing (not how hardware does it)
	// technically a oam dma should delay a cycle before writing
	if(dma_address < 0xe000)
	{
		oam_dma_active = false;
		// old implementaiton
		for(int i = 0; i < 0xA0; i++)
		{
			oam[i] =  read_mem(dma_address+i); 	
		}
		oam_dma_active = true; // indicate a dma is active and to lock memory
		oam_dma_address = dma_address; // the source address
		oam_dma_index = 0; // how far along the dma transfer we are		
	}
}

// this needs work
void Memory::tick_dma(int cycles) noexcept
{
    // not active do nothing!
    if(!oam_dma_active) 
    {
        return;
    }

	oam_dma_index += cycles;
	// We are done with our dma
	if(oam_dma_index >= 0xa2) 
	{
		oam_dma_active = false;
		return;
	}	
}


// io memory has side affects 0xff00
void Memory::write_io(uint16_t addr,uint8_t v) noexcept
{
    switch(addr & 0xff)
    {
		// serial data (current we just use this for testing purposes)
		case IO_SB:
		{
			if(test_result == emu_test::running)
			{
				// handle gekkios
				if(v == 0x42)
				{
					if(++gekkio_fail_count == 6)
					{
						test_result = emu_test::fail;
					}
				}

				else if(gekkio_pass_count < 6 && v == gekkio_pass_magic[gekkio_pass_count])
				{
					gekkio_pass_count++;
					if(gekkio_pass_count == 6)
					{
						test_result = emu_test::pass;
					}
				}
			}
			break;
		}

		// unused io
		case 0x03:
		{
			break;
		}
			
		// unused
		case 0x08: case 0x09: case 0x0a: case 0x0b:
		case 0x0c: case 0x0d: case 0x0e: case 0x15:
		case 0x1f: case 0x27: case 0x28: case 0x29:
		{
			io[addr & 0xff] = 0xff;
			break;
		}
			

		// update the timer freq (tac in gb docs)
		case IO_TMC:
		{
			bool is_set = cpu->internal_tima_bit_set();

			bool enabled = cpu->tima_enabled();

			io[IO_TMC] = v | 248;

			// as our edge is anded with the enable
			// disalbing it when enabled can cause a drop from
			// high to low
			if(enabled && is_set && !cpu->tima_enabled())
			{
				cpu->tima_inc();
			}

			// changing the freq can cause the bit to drop from high to low
			if((is_set && !cpu->internal_tima_bit_set() && cpu->tima_enabled()))
			{
				cpu->tima_inc();
			}

			break;
		}		

		// div and tima share the same internal counter
		// should account for this internal behavior
		case IO_DIV:
		{
			if(cpu->internal_tima_bit_set() && cpu->tima_enabled())
			{
				cpu->tima_inc();
			}

			cpu->internal_timer = 0;
			break;
		}
			
		case IO_IF:
		{
			io[IO_IF] = v | (128 + 64 + 32); // top 3 bits allways on
			break;
		}

		case IO_LCDC: // lcdc
		{
			if(!is_set(v,7) && is_lcd_enabled()) // lcd switched off this write
			{
				ppu->turn_lcd_off();
			}
			
			if(is_set(v,7) && !is_lcd_enabled())
			{
				ppu->set_scanline_counter(0);
				io[IO_STAT] |= 2; // mode 2?
				ppu->mode = ppu_mode::oam_search;	
			}
			io[IO_LCDC] = v;
			ppu->stat_update();
			break;
		}


		// lcd stat <-- writes can trigger interrupts?
		case IO_STAT:
		{
			// delete writeable bits
			io[IO_STAT] &= 7;
				
			// set the w bits with the value
			io[IO_STAT] |= v & ~7;
			
			// set unused bit
			io[IO_STAT] |= 0x80;
			ppu->write_stat();
			break;
		}


		// block ly writes
		case IO_LY:
		{
			break;
		}

		case IO_LYC:
		{
			io[IO_LYC] = v;
			ppu->stat_update();
			break;
		}

		// implement timing on dma and page boundary checking
		case IO_DMA: // dma reg perform a dma transfer //<-- may need seperate handling in a do_dma
		{
			do_dma(v);
			break;
		}

		// as soon as this is written to disable the bios
		case IO_BIOS:
		{
			bios_disable();
			break;
		}

		// cgb regs
		
		// cgb ram bank number
		case IO_SVBK:
		{
			if(cpu->get_cgb())
			{
				cgb_wram_bank_idx = v & 0x7;
				
				// bank 0 is same as accessing bank 1
				if(cgb_wram_bank_idx == 0)
				{
					cgb_wram_bank_idx = 1;
				}
				
				// index it
				cgb_wram_bank_idx -= 1;
				
				io[IO_SVBK] = v | 248;
				
			}
			
			else
			{
				io[IO_SVBK] = v;
			}
			
			break;
		}

		case IO_SPEED:
		{
			// not cgb return ff 
			if(!cpu->get_cgb())
			{
				io[IO_SPEED] = 0xff;
			}

			else
			{
				io[IO_SPEED] = (io[IO_SPEED] & 0x80) | (v & 0x1) | 0x7e;
			}
			break;
		}

		case IO_VBANK: // what vram bank are we writing to?
		{
			// not cgb return data
			if(!cpu->get_cgb())
			{
				io[IO_VBANK] = v;
			}
			
			else
			{
				vram_bank = v & 1;
				io[IO_VBANK] = v | 254;
			}
			break;
		}

		case IO_BGPI:
		{
			// not cgb return ff 
			if(!cpu->get_cgb())
			{
				io[IO_BGPI] = v;
			}
			
			else
			{
				ppu->set_bg_pal_idx(v);
				io[IO_BGPI] = v | 0x40;
			}
			break;
		}

		case IO_BGPD: // finish later 
		{
			if(!cpu->get_cgb())
			{
				io[IO_BGPD] = v;
			}

			else
			{
				ppu->write_bgpd(v);
			}
			break;
		}

		case IO_SPPI: // sprite pallete index
		{
			// not cgb return ff 
			if(!cpu->get_cgb())
			{
				io[IO_SPPI] = v;
			}			

			else
			{
				ppu->set_sp_pal_idx(v);
				io[IO_SPPI] = v | 0x40;
			}
			break;
		}		
		
		case IO_SPPD: // sprite pallete data
		{
			// not cgb return ff 
			if(!cpu->get_cgb())
			{
				io[IO_SPPD] = v;
			}

			else
			{			
				ppu->write_sppd(v);
			}
			break;
		}
	
		// specifies src byte dest of dma
		case IO_HDMA1:
		{
			dma_src &= 0xff;
			dma_src |= v << 8;
			io[IO_HDMA1] = v;
			break;
		}
		
		// lo byte dma src
		case IO_HDMA2:
		{
			v &= 0xf0;
			dma_src &= ~0xff;
			dma_src |= v;
			io[IO_HDMA2] = v;
			break;
		}
		
		
		// high byte dma dst
		case IO_HDMA3:
		{
			v &= 0x1f;
			dma_dst &= 0xff;
			dma_dst |= v << 8;
			io[IO_HDMA3] = v;
			break;
		}
		
		// low byte dma dst
		case IO_HDMA4:
		{
			v &= 0xf0;
			dma_dst &= ~0xff;
			dma_dst |= v;
			io[IO_HDMA4] = v;
			break;
		}
	
		// cgb dma start
		case IO_HDMA5: // <-- unsure on this behavior
		{
			io[IO_HDMA5] = v;
			// if bit 7	is zero do a gdma
			// 1 will start a hdma during hblank
			
			// if data is 0 and there is no active hdma
			if(!is_set(v,7) && !hdma_active) 
			{
				do_gdma();
			}
			
			// writing 0 to bit 7 terminates a hdma transfer
			else if(!is_set(v,7))
			{
				hdma_active = false;
			}
			
			else // start a hdma
			{
				// number of 16 byte incremnts to transfer
				hdma_len = (v & 0x7f)+1;
				hdma_len_ticked = 0;
				hdma_active = true;
			}
			break;
		}


		// sound registers
		case IO_NR10:
		{
			if(apu->enabled())
			{
				apu->c1.sweep_write(v);
				io[IO_NR10] = v | 128;
			}
			break;
		}



		case IO_NR11:
		{
			if(apu->enabled())
			{
				// bottom 6 bits are length data 
				// set the internal counter to 64 - bottom 6 bits of data
				apu->c1.write_lengthc(v);
				apu->c1.write_cur_duty(v);
				io[IO_NR11] = v;
			}
			break;
		}

		case IO_NR12:
		{
			if(apu->enabled())
			{
				io[IO_NR12] = v;
				apu->c1.check_dac();
				apu->c1.env_write(v);
			}
			break;
		}

		case IO_NR13:
		{
			if(apu->enabled())
			{
				apu->c1.freq_write_lower(v);
				io[IO_NR13] = v;
			}
			break;
		}

		case IO_NR14:
		{
			if(apu->enabled())
			{
				apu->c1.freq_write_higher(v);
				

				if(is_set(v,7)) // trigger
				{
					apu->c1.length_trigger();
					apu->c1.freq_reload_period();
					apu->c1.env_trigger();
					apu->c1.sweep_trigger();
					apu->c1.duty_trigger();
				}

				apu->c1.length_write(v,apu->get_sequencer_step());

				io[IO_NR14] = v;
			}
			break;
		}



		case IO_NR21:
		{
			if(apu->enabled())
			{
				apu->c2.write_lengthc(v);
				apu->c2.write_cur_duty(v);
				io[IO_NR21] = v;
			}
			break;
		}

		case IO_NR22:
		{
			if(apu->enabled())
			{
				io[IO_NR22] = v;
				apu->c2.check_dac();	
				apu->c2.env_write(v);
			}
			break;
		}

		case IO_NR23:
		{
			if(apu->enabled())
			{
				io[IO_NR23] = v;
				apu->c2.freq_write_lower(v);
			}
			break;
		}

		case IO_NR24:
		{
			if(apu->enabled())
			{
				apu->c2.freq_write_higher(v);

				if(is_set(v,7)) // trigger
				{
					apu->c2.length_trigger();
					apu->c2.freq_reload_period();
					apu->c2.env_trigger();
					apu->c2.duty_trigger();
				}

				apu->c2.length_write(v,apu->get_sequencer_step());

				io[IO_NR24] = v;
			}
			break;
		}


		case IO_NR30:
		{
			if(apu->enabled())
			{
				io[IO_NR30] = v | 127;
				apu->c3.check_dac();
			}
			break;
		}


		case IO_NR31:
		{
			if(apu->enabled())
			{
				apu->c3.write_lengthc(v);
				io[IO_NR31] = v;
			}
			break;
		}

		case IO_NR32:
		{
			if(apu->enabled())
			{
				apu->c3.write_vol(v);
				io[IO_NR32] = v | 159;
			}
			break;
		}

		case IO_NR33:
		{
			if(apu->enabled())
			{
				apu->c3.freq_write_lower(v);
				io[IO_NR33] = v;
			}
			break;
		}

		case IO_NR34:
		{
			if(apu->enabled())
			{
				apu->c3.freq_write_higher(v);

				if(is_set(v,7)) // trigger
				{
					apu->c3.length_trigger();
					apu->c3.freq_reload_period();
					apu->c3.wave_trigger();
					apu->c3.vol_trigger();
				}

				apu->c3.length_write(v,apu->get_sequencer_step());

				// after all the trigger events have happend
				// if the dac is off switch channel off				
				apu->c3.check_dac();


				io[IO_NR34] = v | (16 + 32 + 8);
			}
			break;
		}

		case IO_NR41:
		{
			if(apu->enabled())
			{
				apu->c4.write_lengthc(v);
				io[IO_NR41] = v | 192;
			}
			break;
		}

		case IO_NR42:
		{
			if(apu->enabled())
			{
				io[IO_NR42] = v;
				apu->c4.check_dac();
				apu->c4.env_write(v);
			}
			break;
		}

		case IO_NR43:
		{
			if(apu->enabled())
			{
				io[IO_NR43] = v;
				apu->c4.noise_write(v);
			}
			break;
		}

		case IO_NR44:
		{


			if(apu->enabled())
			{
				if(is_set(v,7)) // trigger
				{
					apu->c4.length_trigger();
					apu->c4.env_trigger();
					apu->c4.noise_trigger();
				}

				apu->c4.length_write(v,apu->get_sequencer_step());		

				apu->c4.check_dac();	

				io[IO_NR44] = v | 63;
			}
			break;
		}		


		// nr 50
		case IO_NR50:
		{
			if(apu->enabled())
			{
				io[IO_NR50] = v;
			}
			break;
		}

			
		case IO_NR51:
		{
			if(apu->enabled())
			{
				io[IO_NR51] = v;
			}
			break;
		}



		// bits 0-3 read only 7 r/w 4-6 unused
		case IO_NR52:
		{
			io[IO_NR52] |= 112;
			
			if(is_set(io[IO_NR52] ^ v,7)) // if we are going from on to off reset the sequencer
			{
				apu->reset_sequencer();
			}



			// if we have disabled sound we should
			// zero all the registers (nr10-nr51) 
			// and lock writes until its on
			if(!is_set(v,7))
			{
				apu->disable_sound();
			}
			
			else // its enabled
			{
				apu->enable_sound();
			}			
			break;
		}


		// wave table
		case 0x30: case 0x31: case 0x32: case 0x33:
		case 0x34: case 0x35: case 0x36: case 0x37:
		case 0x38: case 0x39: case 0x3a: case 0x3b:
		case 0x3c: case 0x3d: case 0x3e: case 0x3f:
		{
			// if wave is on write to current byte <-- finish accuracy later
			if(apu->chan_enabled(2))
			{
				io[0x30 + (apu->c3.get_duty_idx() / 2)] = v;
			}
			
			else // if its off allow "free reign" over it
			{
				io[addr & 0xff] = v;	
			}
			return;
		}


        default: // hram
        {
            io[addr & 0xff] = v;
            return;
        }
    }
}


void Memory::do_gdma() noexcept
{
	const uint16_t source = dma_src & 0xfff0;
	
	const uint16_t dest = (dma_dst & 0x1ff0) | 0x8000;
	
	// hdma5 stores how many 16 byte incremnts we have to transfer
	const int len = ((io[IO_HDMA5] & 0x7f) + 1) * 0x10;

	
	// find out how many cycles we tick but for now just copy the whole damb thing 
	for(int i = 0; i < len; i++)
	{
		write_mem(dest+i,read_mem(source+i));
	}

	cpu->cycle_tick(8*(len / 0x10)); // 8 M cycles for each 10 byte block

	io[IO_HDMA5] = 0xff; // terminate the transfer
}


void Memory::do_hdma() noexcept
{

	if(!hdma_active)
	{
		return;
	}

	const uint16_t source = (dma_src & 0xfff0) + hdma_len_ticked*0x10;

	const uint16_t dest = ((dma_dst & 0x1ff0) | 0x8000) + hdma_len_ticked*0x10;

	/*if(!(source <= 0x7ff0 || ( source >= 0xa000 && source <= 0xdff0)))
	{
		printf("ILEGGAL HDMA SOURCE: %X!\n",source);
		exit(1);
	}
	*/

	// find out how many cycles we tick but for now just copy the whole damb thing 						
	for(int i = 0; i < 0x10; i++)
	{
		write_mem(dest+i,read_mem(source+i));
	}

	// 8 M cycles for each 0x10 block
	cpu->cycle_tick(8);
	
	// hdma is over 
	if(--hdma_len <= 0)
	{
		// indicate the tranfser is over
		io[IO_HDMA5] = 0xff;
		hdma_active = false;
	}

	// goto next block
	else
	{
		hdma_len_ticked++;
	}	
}

void Memory::write_iot(uint16_t addr,uint8_t v) noexcept
{
    write_io(addr,v);
    cpu->cycle_tick(1); // tick for mem access
}

// wram zero bank 0xc000 - 0xd000
void Memory::write_wram_low(uint16_t addr,uint8_t v) noexcept
{
    wram[addr&0xfff] = v;
}

// banked wram 0xd000 - 0xe000
// also at 0xe000 - 0xfe00 in echo ram
void Memory::write_wram_high(uint16_t addr,uint8_t v) noexcept
{
    cgb_wram_bank[cgb_wram_bank_idx][addr&0xfff] = v;
}

// high ram 0xf000
// we bundle io into this but the hram section is at 0xff80-ffff
void Memory::write_hram(uint16_t addr,uint8_t v) noexcept
{
    // io regs
    if(addr >= 0xff00)
    {
        write_io(addr,v);
    }

    // high wram mirror
    else if(addr >= 0xf000 && addr <= 0xfdff)
    {
        write_wram_high(addr,v);
    }

	// oam is accesible during mode 0-1
	else if(addr >= 0xfe00 && addr <= 0xfe9f)
	{
		write_oam(addr,v);
		return;
	}


    else // restricted
    {

    }
}


void Memory::write_cart_ram(uint16_t addr, uint8_t v) noexcept
{
    if(enable_ram && cart_ram_bank != CART_RAM_BANK_INVALID)
    {
        cart_ram_banks[cart_ram_bank][addr & 0x1fff] = v;
    }
}

// only bottom 4 bits are readable
void Memory::write_cart_ram_mbc2(uint16_t addr, uint8_t v) noexcept
{
    if(enable_ram) // fixed for 512by4 bits
    {
        cart_ram_banks[0][addr & 0x1ff] = ((v & 0xf) | 0xf0);
    }
}


}