#include <gb/gb.h>
#include <gb/cpu.inl>
#include <albion/lib.h>
#include <albion/debug.h>
#include <gb/sgb.h>

// todo on what cycles are read and writes asserted on the bus?
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

	const auto save_name = get_save_file_name(filename);

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

	const auto save_name = get_save_file_name(filename);

    auto fp = std::fstream(save_name, std::ios::in | std::ios::binary);
	if(fp)
	{
		for(auto &x : cart_ram_banks)
		{
			fp.read(reinterpret_cast<char*>(x.data()), x.size());
		}
		fp.close();	
	}
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


bool Memory::rom_sgb_enabled() const noexcept
{
	return rom[0x146] == 0x3;
}


Memory::Memory(GB &gb) : cpu(gb.cpu), ppu(gb.ppu), 
	apu(gb.apu), scheduler(gb.scheduler), debug(gb.debug)
{
	// reserve our underlying memory
    cgb_wram_bank.resize(7);
    for(auto &x: cgb_wram_bank)
    {
        x.resize(0x1000);
		std::fill(x.begin(),x.end(),0);
    }

    wram.resize(0x1000);
	std::fill(wram.begin(),wram.end(),0); 
    oam.resize(0xa0);
	std::fill(oam.begin(),oam.end(),0);
    io.resize(0x100);
	std::fill(io.begin(),io.end(),0);

    vram.resize(0x2);
    for(auto &x: vram)
    {
        x.resize(0x2000); 
		std::fill(x.begin(),x.end(),0);
    }
	rom.resize(0x8000);

	sgb_packet.resize(111);

	sgb_pal.resize(0x1000);
} 

void Memory::init_mem_table() noexcept
{
	memory_table[0x8].read_memf = &Memory::read_vram;
	memory_table[0x9].read_memf = &Memory::read_vram;
	memory_table[0xa].read_memf = &Memory::read_cart_ram;
	memory_table[0xb].read_memf = &Memory::read_cart_ram;
	memory_table[0xc].read_memf = &Memory::read_wram_low;
	memory_table[0xd].read_memf = &Memory::read_wram_high;
	memory_table[0xe].read_memf = &Memory::read_wram_low;
	memory_table[0xf].read_memf = &Memory::read_hram;

	// init page table
	unlock_vram();
	page_table[0xa] = nullptr;
	page_table[0xb] = nullptr;
	page_table[0xc] = wram.data();
	page_table[0xd] = &cgb_wram_bank[0][0];
	page_table[0xe] = wram.data();
	page_table[0xf] = nullptr;


    // write mem
	memory_table[0x8].write_memf = &Memory::write_vram;
	memory_table[0x9].write_memf = &Memory::write_vram;
	memory_table[0xa].write_memf = &Memory::write_cart_ram;
	memory_table[0xb].write_memf = &Memory::write_cart_ram;
	memory_table[0xc].write_memf = &Memory::write_wram_low;
	memory_table[0xd].write_memf = &Memory::write_wram_high;
	memory_table[0xe].write_memf = &Memory::write_wram_low;
	memory_table[0xf].write_memf = &Memory::write_hram;	
}

void Memory::init_banking_table() noexcept
{
    // banking read mem
	memory_table[0x0].read_memf = &Memory::read_bank_zero;
	memory_table[0x1].read_memf = &Memory::read_bank_zero;
	memory_table[0x2].read_memf = &Memory::read_bank_zero;
	memory_table[0x3].read_memf = &Memory::read_bank_zero;
	memory_table[0x4].read_memf = &Memory::read_rom_bank;
	memory_table[0x5].read_memf = &Memory::read_rom_bank;
	memory_table[0x6].read_memf = &Memory::read_rom_bank;
	memory_table[0x7].read_memf = &Memory::read_rom_bank;

	update_page_table_bank();
	update_page_table_sram();


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
			memory_table[0x0].write_memf = &Memory::ram_bank_enable_mbc5;
			memory_table[0x1].write_memf = &Memory::ram_bank_enable_mbc5;
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
	}
}

void Memory::init(std::string rom_name, bool with_rom, bool use_bios)
{
	if(with_rom)
	{
		read_bin(rom_name,rom); // read our rom in

		// if we have a ips patch with the same name try loading it
		const auto ips_file = remove_ext(rom_name) + ".ips";

		load_ips_patch(ips_file,rom);


		// propagate an error back with an exception later for now we just bail
		if(rom.size() < 0x4000)
		{
			throw std::runtime_error("rom is too small!");
		}
	}

	else
	{
		rom.resize(0x8000);
	}

	if(use_bios)
	{
		std::string bios_file = rom_cgb_enabled() ? "gbc_bios.bin" : "dmg_bios.bin";

		if(read_bin(bios_file,bios))
		{
			throw std::runtime_error(fmt::format("could not load bios file: {}!",bios_file));
		}
	}

    // init memory
    vram.resize(0x2);
    for(auto &x: vram)
    {
		std::fill(x.begin(),x.end(),0);
    }
	std::fill(wram.begin(),wram.end(),0); 
	std::fill(oam.begin(),oam.end(),0);
	std::fill(io.begin(),io.end(),0);
    for(auto &x: cgb_wram_bank)
    {
		std::fill(x.begin(),x.end(),0);
    }


    // pull out our rom info
    rom_info.init(rom,rom_name);


    cart_ram_banks.resize(rom_info.no_ram_banks);
    for(auto &x: cart_ram_banks)
    {
        x.resize(0x2000);
		std::fill(x.begin(),x.end(),0);
    }
	load_cart_ram();


	if(!use_bios)
	{
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

	ignore_oam_bug = false;


	// sgb
    sgb_transfer_active = false;
    packet_count = 0;
    packet_len = 0;
    bit_count = 0;

	test_result = emu_test::running;
	gekkio_pass_count = 0;
	gekkio_fail_count = 0;

    // init our function table
	init_mem_table();

	init_banking_table();

	if(use_bios)
	{
		for(int i = 0; i < 8; i++)
		{
			page_table[i] = nullptr;
		}
	}
}


void Memory::lock_vram()
{
	page_table[0x8] = nullptr;
	page_table[0x9] = nullptr;
}

void Memory::unlock_vram()
{
	page_table[0x8] = &vram[vram_bank][0];
	page_table[0x9] = &vram[vram_bank][0x1000];
}


u8 Memory::read_bios(u16 addr) const noexcept
{
	if(cpu.is_cgb)
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

void Memory::raw_write_word(u16 addr, u16 v) noexcept
{
	raw_write(addr,v & 0xff);
	raw_write(addr+1,(v >> 8) & 0xff);
}


void Memory::raw_write(u16 addr, u8 v) noexcept
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
				write_io(addr,v);
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

u16 Memory::raw_read_word(u16 addr) const noexcept
{
	return raw_read(addr) | raw_read(addr+1) << 8;
}

u8 Memory::raw_read(u16 addr) const noexcept
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

// read mem
#ifdef DEBUG
u8 Memory::read_mem_debug(u16 addr) const noexcept
{
	const u8 value = read_mem_no_debug(addr);
	if(debug.breakpoint_hit(addr,value,break_type::read))
	{
		// halt until told otherwhise :)
		write_log(debug,"[DEBUG] read breakpoint hit ({:x}:{:x})",addr,value);
		debug.halt();
	}
	return value;
}
#endif


u8 Memory::read_mem_no_debug(u16 addr) const noexcept
{
	const auto idx = (addr & 0xf000) >> 12;
	if(page_table[idx] != nullptr)
	{
		const u8 *buf = page_table[idx] + (addr & 0xfff);
		return *buf;
	}
	
    return std::invoke(memory_table[idx].read_memf,this,addr);
}


// write_mem

#ifdef DEBUG
void Memory::write_mem_debug(u16 addr, u8 v) noexcept
{
	if(debug.breakpoint_hit(addr,v,break_type::write))
	{
		// halt until told otherwhise :)
		write_log(debug,"[DEBUG] write breakpoint hit ({:x}:{:})",addr,v);
		debug.halt();
	}

	write_mem_no_debug(addr,v);
}
#endif

void Memory::write_mem_no_debug(u16 addr, u8 v) noexcept
{
	std::invoke(memory_table[(addr & 0xf000) >> 12].write_memf,this,addr,v);	
}


u16 Memory::read_word(u16 addr) noexcept
{
    return read_mem(addr) | (read_mem(addr+1) << 8);
}


void Memory::write_word(u16 addr, u16 v) noexcept
{
    write_mem(addr+1,(v&0xff00)>>8);
    write_mem(addr,(v&0x00ff));
}

// maybe should have an eqiv for optimisation purposes where we know it cant trigger
u8 Memory::read_memt_no_oam_bug(u16 addr) noexcept
{
	ignore_oam_bug = true;
	u8 v = read_mem(addr);
	ignore_oam_bug = false;
	cpu.cycle_tick(1); // tick for the memory access 
    return v;
}

// memory accesses (timed)
u8 Memory::read_memt(u16 addr) noexcept
{
    u8 v = read_mem(addr);
	cpu.cycle_tick(1); // tick for the memory access 
    return v;
}

void Memory::write_memt_no_oam_bug(u16 addr, u8 v) noexcept
{
	ignore_oam_bug = true;
    write_mem(addr,v);
	ignore_oam_bug = false;
	cpu.cycle_tick(1); // tick for the memory access	
}


void Memory::write_memt(u16 addr, u8 v) noexcept
{
    write_mem(addr,v);
	cpu.cycle_tick(1); // tick for the memory access
}


u16 Memory::read_wordt(u16 addr) noexcept
{
    return read_memt(addr) | (read_memt(addr+1) << 8);
}


void Memory::write_wordt(u16 addr, u16 v) noexcept
{
    write_memt(addr+1,(v&0xff00)>>8);
    write_memt(addr,(v&0x00ff));
}


// read mem underyling
// object attribute map 0xfe00 - 0xfe9f
u8 Memory::read_oam(u16 addr) const noexcept
{
	if(!ignore_oam_bug)
	{
		cpu.oam_bug_read(addr);
	}

	if(addr >= 0xfea0)
	{
		return 0xff;
	}

    // cant access oam during a dma
    if(oam_dma_active)
    {
        return 0xff;
    }

    // if not in vblank or hblank cant access it
    if(ppu.get_mode() != ppu_mode::hblank && ppu.get_mode() != ppu_mode::vblank && !ppu.glitched_oam_mode)
    {
        return 0xff;
    }

    return oam[addr & 0xff];
}

// video ram 0x8000 - 0xa000
u8 Memory::read_vram(u16 addr) const noexcept
{
	scheduler.service_events();
    // vram is used in pixel transfer cannot access
    if(ppu.get_mode() != ppu_mode::pixel_transfer)
    {
        return vram[vram_bank][addr & 0x1fff];
    }

    else
    {
        return 0xff;
    }
}

// cart  memory 0xa000 - 0xc000
u8 Memory::read_cart_ram(u16 addr) const noexcept
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
u8 Memory::read_cart_ram_mbc2(u16 addr) const noexcept
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
u8 Memory::read_io(u16 addr) const noexcept
{
	//printf("%x\n",addr & 0xff);
    switch(addr & 0xff)
    {
        // joypad control reg <-- used for sgb command packets too
		case IO_JOYPAD:
		{		
			// read from mem
			const u8 req = io[IO_JOYPAD];
			// we want to test if bit 5 and 4 are set 
			// so we can determine what the game is interested
			// in reading
						
				
			// read out dpad 
			if(!is_set(req,4))
			{
				return ( (req & 0xf0) | (cpu.joypad_state & 0x0f) | 0xc0 );
			}
			// read out a b sel start 
			else if(!is_set(req,5))
			{
				return ( (req & 0xf0) | ((cpu.joypad_state >> 4) & 0xf ) | 0xc0 );
			}		
				
			return 0xf0; // return all unset
		}	

		case IO_SC:
		{
			return io[IO_SC] | (cpu.is_cgb? 0x7d : 0x7e);
		}

		case IO_STAT:
		{
			return ppu.read_stat();
		}

        case IO_LY:
        {
            return ppu.get_current_line();
        }

		case IO_DIV:
		{
			// reinsert the event so div can update
			scheduler.remove(gameboy_event::internal_timer);
			cpu.insert_new_timer_event();

			// div register is upper 8 bits of the internal timer
			return (cpu.internal_timer & 0xff00) >> 8;
		}

		case IO_NR10:
		{
			return apu.psg.read_nr10();
		}

		// sound regs
		// nr 11 only 7 and 6 readable
		case IO_NR11:
		{
			return apu.psg.read_nr11();
		}
		
		case IO_NR12:
		{
			return apu.psg.read_nr12();
		}

		// write only
		case IO_NR13:
		{
			return 0xff;
		}
			
		// nr 14 only 6 is readable
		case IO_NR14:
		{
			return apu.psg.read_nr14();
		}
			
		// nr 21 only bits 6-7 are r 
		case IO_NR21:
		{
			return apu.psg.read_nr21();	
		}
			
		case IO_NR22:
		{
			return apu.psg.read_nr22();
		}

		// nr 23 write only
		case IO_NR23:
		{
			return 0xff;
		}
			
		// nr 24 only bit 6 can be read 
		case IO_NR24:
		{
			return apu.psg.read_nr24();
		}
			
		// nr 30 only bit 7
		case IO_NR30:
		{
			return apu.psg.read_nr30();	
		}
			
		// nr 31 <-- unsure
		case IO_NR31:
		{
			return 0xff;
		}
			
		// nr 32 6-5 r
		case IO_NR32:
		{
			return apu.psg.read_nr32();
		}
			
		// nr33 write only
		case IO_NR33:
		{
			return 0xff;
		}
			
		// nr 34 6 r
		case IO_NR34:
		{
			return apu.psg.read_nr34();
		}
			
		// nr 41
		case IO_NR41:
		{
			return 0xff;
		}

		case IO_NR42:
		{
			return apu.psg.read_nr42();
		}

		case IO_NR43:
		{
			return apu.psg.read_nr43();
		}

		// nr 44 bit 6
		case IO_NR44:
		{
			return apu.psg.read_nr44();	
		}		


		case IO_NR50:
		{
			return apu.psg.read_nr50();
		}

		case IO_NR51:
		{
			return apu.psg.read_nr51();
		}

		case IO_NR52:
		{
			return apu.psg.read_nr52();
		}

		// unused
		case 0x2a: case 0x2b: case 0x2c: 
		case 0x2d: case 0x2e: case 0x2f:
		case 0x4c: case 0x4e: case 0x50:
		case 0x57: case 0x58:
		case 0x59: case 0x5a: case 0x5b:
		case 0x5c: case 0x5d: case 0x5e:
		case 0x5f: case 0x60: case 0x61:
		case 0x62: case 0x63: case 0x64:
		case 0x65: case 0x66: case 0x67:
		case 0x6d: case 0x6e: case 0x6f:
		case 0x71: case 0x78: case 0x79:
		case 0x7a: case 0x7b: case 0x7c:
		case 0x7d: case 0x7e: case 0x7f:
		{
			return 0xff;
		}


		case IO_HDMA1: case IO_HDMA2: case IO_HDMA3:
		case IO_HDMA4:
		{
			if(!cpu.is_cgb)
			{
				return 0xff;
			}

			return io[addr & 0xff];
		}

		// wave table
		case 0x30: case 0x31: case 0x32: case 0x33:
		case 0x34: case 0x35: case 0x36: case 0x37:
		case 0x38: case 0x39: case 0x3a: case 0x3b:
		case 0x3c: case 0x3d: case 0x3e: case 0x3f:
		{
			return apu.psg.read_wave_table(addr-0xff30);
		}



		// CGB
		
		
		
		// cgb vram bank unused on dmg
		case IO_VBANK:
		{
			if(cpu.is_cgb)
			{
				return io[IO_VBANK];
			}
			return 0xff;
		}

		
		case IO_BGPD: 
		{
			if(cpu.is_cgb)
			{
				return ppu.get_bgpd();
			}
			return 0xff;
		}
		
		case IO_BGPI: 
		{
			if(cpu.is_cgb)
			{
				return io[IO_BGPI];
			}
			return 0xff;
		}		

		case IO_SPPD:
		{
			if(cpu.is_cgb)
			{
				return ppu.get_sppd();
			}
			return 0xff;
		}

		case IO_SPPI: 
		{
			if(cpu.is_cgb)
			{
				return io[IO_SPPI];
			}
			return 0xff;
		}		
		
		case IO_HDMA5: // verify
		{
			if(cpu.is_cgb)
			{
				if(!hdma_active)
				{
					return 0xff;
				}
				
				// return the lenght tick left
				return (hdma_len - 1);
			}

			return 0xff;
		}

		// infared communications
		case IO_RP:
		{	
			// stubbed
			if(cpu.is_cgb)
			{
				return io[IO_RP];
			}

			return 0xff;
		}

		case IO_OPRI: // unknown
		{
			if(cpu.is_cgb)
			{
				return io[IO_OPRI] | 0xfe;
			}
			return 0xff;
		}

		case 0x72: // unknown
		{
			if(cpu.is_cgb)
			{
				return io[0x72];
			}
			return 0xff;
		}

		case 0x73: // unknown
		{
			if(cpu.is_cgb)
			{
				return io[0x73];
			}
			return 0xff;
		}

		case 0x74: // unknown
		{
			if(cpu.is_cgb)
			{
				return io[0x74];
			}
			return 0xff;
		}

		case 0x75: // unknown
		{
			if(cpu.is_cgb)
			{
				return io[0x75];
			}
			return 0xff;
		}


		case IO_PCM12:
		{
			if(cpu.is_cgb)
			{
				return apu.psg.channels[0].output | apu.psg.channels[1].output << 4;
			}
			return 0xff;
		}

		case IO_PCM34:
		{
			if(cpu.is_cgb)
			{
				return apu.psg.channels[2].output | apu.psg.channels[3].output << 4;
			}
			return 0xff;
		}


        default: 
        {
            return io[addr & 0xff];
        }
    }
}

#ifdef DEBUG
u8 Memory::read_iot_debug(u16 addr) noexcept
{
	const u8 value = read_iot_no_debug(addr);
	if(debug.breakpoint_hit(addr,value,break_type::read))
	{
		// halt until told otherwhise :)
		write_log(debug,"[DEBUG] read breakpoint hit ({:x}:{:x})",addr,value);
		debug.halt();
	}
	return value;	
}
#endif

u8 Memory::read_iot_no_debug(u16 addr) noexcept
{
	scheduler.service_events();
    u8 v = read_io(addr);
	cpu.cycle_tick(1); // tick for mem access
    return v;
}

// for now we will just return the rom
// 0x4000 - 0x8000 return current rom bank
u8 Memory::read_rom_bank(u16 addr) const noexcept
{  
	return rom[cart_rom_bank * 0x4000 + (addr & 0x3fff)];
}



u8 Memory::read_rom_lower_mbc1(u16 addr) const noexcept
{
	if(rom_banking) // in rom banking its allways bank zero
	{
		return rom[addr & 0x3fff];
	}

	
	// else the bank2 register is used shift left by 5 bits
	// which is hte top 2 bits of our cart_rom_bank allready :)
	else // TODO optimise this bank read
	{

		return rom[((cart_rom_bank & ~0x1f) * 0x4000) +  (addr & 0x3fff)];
	}
}

//0x0000 - 0x4000 return rom bank zero
u8 Memory::read_bank_zero(u16 addr) const noexcept
{
    return rom[addr];
}

// 0xc000 && 0xe000 (echo ram)
// return wram non banked
u8 Memory::read_wram_low(u16 addr) const noexcept
{
    return wram[addr&0xfff];
}

// 0xd000
// return banked wram this is fixed on dmg
u8 Memory::read_wram_high(u16 addr) const noexcept
{
    return cgb_wram_bank[cgb_wram_bank_idx][addr&0xfff];
}

// 0xf000 various
u8 Memory::read_hram(u16 addr) const noexcept
{
	scheduler.service_events();
    // io regs
    if(addr >= 0xff00)
    {
        return read_io(addr);
    }

    // high wram mirror
    else if(addr >= 0xf000 && addr <= 0xfdff)
    {
        return std::invoke(memory_table[0xd].read_memf,this,addr);
    }

	// oam is accesible during mode 0-1
	else if(addr >= 0xfe00 && addr <= 0xfeff)
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
void Memory::write_oam(u16 addr,u8 v) noexcept
{
	if(!ignore_oam_bug)
	{
		cpu.oam_bug_write(addr);
	}

	if(addr >= 0xfea0)
	{
		return;
	}

    if(oam_dma_active)
    {
        return;
    }

    // if not in vblank or hblank cant access it
    if(ppu.get_mode() != ppu_mode::hblank && ppu.get_mode() != ppu_mode::vblank && !ppu.glitched_oam_mode)
    {
        return;
    }

    oam[addr & 0xff] = v;    
}

//video ram 0x8000 - 0xa000
void Memory::write_vram(u16 addr,u8 v) noexcept
{
	scheduler.service_events();
    // vram is used in pixel transfer cannot access
    if(ppu.get_mode() != ppu_mode::pixel_transfer)
    {
        vram[vram_bank][addr & 0x1fff] = v;
    }
}


void Memory::do_dma(u8 v) noexcept
{
	scheduler.service_events();
	io[IO_DMA] = v; // write to the dma reg
	u16 dma_address = v << 8;
	// transfer is from 0xfe00 to 0xfea0
			
	/*// must be page aligned revisit later
	if(dma_address % 0x100) return;		
	*/

	// source must be below 0xe000
	// tick immediatly but keep the timing (not how hardware does it)
	// technically a oam dma should delay a cycle before writing
	if(dma_address < 0xe000)
	{
		oam_dma_disable();
		for(int i = 0; i < 0xA0; i++)
		{
			oam[i] = read_mem(dma_address+i); 	
		}
		
		oam_dma_address = dma_address; // the source address
		oam_dma_index = 0; // how far along the dma transfer we are	

		const auto event = scheduler.create_event((0xa0 * 4)+0x8,gameboy_event::oam_dma_end);
		scheduler.insert(event,false);
	
		oam_dma_enable();
	}
}

// this needs work
void Memory::tick_dma(u32 cycles) noexcept
{
    // not active do nothing!
    if(!oam_dma_active) 
    {
        return;
    }

	oam_dma_index += cycles;
	// We are done with our dma
	// 8 extra cycles to start and stop
	// this should not be part of the main time
	// as the dma does not block for all of this...
	// 4 cycles per byte
	if(oam_dma_index >= (0xa0 * 4)+8) 
	{
		oam_dma_disable();
	}	
}

// todo on ppu modes swich out the vram and oam access ptrs
// to remove uneeded checks
void Memory::write_blocked(u16 addr, u8 v) noexcept
{
	UNUSED(addr); UNUSED(v);
}

u8 Memory::read_blocked(u16 addr) const noexcept
{
	UNUSED(addr);
	return 0xff;
}

u8 Memory::read_oam_dma(u16 addr) const noexcept
{
	UNUSED(addr);
	scheduler.service_events();
	// cpu gets back what oam is reading
	// so what we need to do is figure out where the oam dma is
	const auto cycles_opt = scheduler.get_event_ticks(gameboy_event::oam_dma_end);
	if(!cycles_opt)
	{
		printf("dma event not active during read_oam_dma:%x:%x:%x!?\n",oam_dma_active,oam_dma_address,oam_dma_index);
		exit(1);
	}
	// todo for the extra start and stop delay is this still the case?
	const auto dma_idx = cycles_opt.value() / 4;
	return raw_read(oam_dma_address + dma_idx); // todo this function may fail under some cases
}


//https://www.reddit.com/r/EmuDev/comments/5hahss/gb_readwrite_memory_during_an_oam_dma/?utm_medium=android_app&utm_source=share
void Memory::oam_dma_disable() noexcept
{
	// disabled restore the normal memory ptrs
	oam_dma_active = false;

	// are we better off just shoving a bool check in all the memory handlers?
	// this seems kinda heavy
	init_mem_table();
	init_banking_table();
	update_page_table_bank();
	update_page_table_sram();
}

void Memory::oam_dma_enable() noexcept
{
	// indicate a dma is active and to lock memory
	oam_dma_active = true; 

	// ok so we have the external vram bus
	
	// and the internal bus for everything other than the io range
	
	// if oam is accessing that range then the cpu will get back what oam
	// dma is reading
	
	// lock out vram
	if(oam_dma_address >= 0x8000 && oam_dma_address <= 0x9fff)
	{
		memory_table[0x8].write_memf = &Memory::write_blocked;
		memory_table[0x9].write_memf = &Memory::write_blocked;
		memory_table[0x8].read_memf = &Memory::read_oam_dma;
		memory_table[0x9].read_memf = &Memory::read_oam_dma;
	}

	// lock out everything else
	else
	{
		for(int i = 0; i < 8; i++)
		{
			memory_table[i].write_memf = &Memory::write_blocked;
			memory_table[i].read_memf = &Memory::read_oam_dma;
		}

		for(int i = 0xa; i < 0xf; i++)
		{
			memory_table[i].write_memf = &Memory::write_blocked;
			memory_table[i].read_memf = &Memory::read_oam_dma;
		}
	}

	// use fallback handlers during oam dma
	for(int i = 0; i < 16; i++)
	{
		page_table[i] = nullptr;
	}

}

// io memory has side affects 0xff00
void Memory::write_io(u16 addr,u8 v) noexcept
{
    switch(addr & 0xff)
    {

		case IO_JOYPAD:
		{
			io[IO_JOYPAD] = v;

			// transfer start
			if(!sgb_transfer_active && !is_set(v,4) && !is_set(v,5) && cpu.is_sgb)
			{
				sgb_transfer_active = true;
			}



			// command packet
			else if(sgb_transfer_active)
			{
				// ignore reset pulse and high pulse
				if(is_set(v,4) == is_set(v,5))
				{
					return;
				}


				u32 bit = 0;

				// p14 is low tranfser bit is zero
				if(!is_set(v,4))
				{
					bit = 0;
				}

				// p15 is low transfer bit is one
				else if(!is_set(v,5))
				{
					bit = 1;
				}

	
				if(bit_count == (16 * 8))
				{
					// need zero as stop bit
					if(bit != 0)
					{
						return;
					}


					packet_count++;
					bit_count = 0;
					if(packet_count == 1)
					{
						packet_len = sgb_packet[0] & 0b111;
						//printf("packet_len: %d\n",packet_len);
					}



					if(packet_count == packet_len)
					{
						sgb_transfer_active = false;
						packet_len = 0;
						packet_count = 0;

						const u32 command = sgb_packet[0] >> 3;


						//printf("sgb command %x\n",command);

						// for now just implement commands necessary to display a palette
						switch(command)
						{
							case PAL_TRN:
							{
								const auto offset = is_set(io[IO_LCDC],4)?  0x0000 : 0x0800;
								memcpy(&sgb_pal[0],&vram[0][offset],0x1000);

								

								break;
							}

							case MASK_EN:
							{
								//printf("mode %d\n",sgb_packet[1]);
								ppu.mask_en = static_cast<Ppu::mask_mode>(sgb_packet[1] & 0x3);
								break;
							}

							case ATTR_SET:
							{
								if(is_set(sgb_packet[1],6))
								{
									ppu.mask_en = Ppu::mask_mode::cancel;
								}
								break;
							}

							case PAL_SET:
							{
								for(int p = 0; p < 3; p++)
								{
									const auto pal = handle_read<u16>(sgb_packet,1 + (p * 2)) & 511;

									for(int i = 0; i < 4; i++)
									{
										const auto c = deset_bit(handle_read<u16>(sgb_pal,(pal*8) + (i * 2)),15);
										ppu.dmg_pal[p][i]  = ppu.col_lut[c];
									}
								}

								if(is_set(sgb_packet[9],6))
								{
									ppu.mask_en = Ppu::mask_mode::cancel;
								}

								break;
							}
							default:  break;
						}
					}
				}

				// store the incomming bit
				else
				{
					const u32 idx = (bit_count / 8) + (packet_count * 16);
					const u32 byte_bit = bit_count % 8;

					sgb_packet[idx] = deset_bit(sgb_packet[idx],byte_bit) | (bit << byte_bit);
					bit_count++;
				}
			}
			break;
		}

		case IO_SC:
		{

			// serial timeout stub does not emulate proper timings
			// NOTE see src_serial from old emulator for full impl
			if(is_set(v,7) && !is_set(io[IO_SC],7) && is_set(v,0))
			{
				if(cpu.is_cgb && is_set(v,1))
				{
					cpu.serial_cyc = 4;
				}
				
				else 
				{
					cpu.serial_cyc = 128;
				}
				cpu.serial_cnt = 8;	

				cpu.insert_new_serial_event();
			}
			io[IO_SC] = v;
			break;			
		}


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
			// tick off the event while its still under the old settings
			scheduler.remove(gameboy_event::internal_timer);

			bool is_set = cpu.internal_tima_bit_set();

			bool enabled = cpu.tima_enabled();

			io[IO_TMC] = v | 248;

			// as our edge is anded with the enable
			// disalbing it when enabled can cause a drop from
			// high to low
			if(enabled && is_set && !cpu.tima_enabled())
			{
				cpu.tima_inc();
			}

			// changing the freq can cause the bit to drop from high to low
			if((is_set && !cpu.internal_tima_bit_set() && cpu.tima_enabled()))
			{
				cpu.tima_inc();
			}

			// dosent matter if the timer is not enabled as the internal one allways is 
			cpu.insert_new_timer_event();
			break;
		}		

		case IO_TIMA:
		{
			io[IO_TIMA] = v;
			break;
		}

		// div and tima share the same internal counter
		// should account for this internal behavior
		case IO_DIV:
		{
			// tick off remaining event as its about to get deset
			scheduler.remove(gameboy_event::internal_timer);

			if(cpu.internal_tima_bit_set() && cpu.tima_enabled())
			{
				cpu.tima_inc();
			}

			cpu.internal_timer = 0;

            // timer reset dump a new event in
            cpu.insert_new_timer_event();
			break;
		}

		case IO_IE:
		{
			io[IO_IE] = v;
			cpu.update_intr_req();
			break;
		}

		case IO_IF:
		{
			io[IO_IF] = v | (128 + 64 + 32); // top 3 bits allways on
			cpu.update_intr_req();
			break;
		}

		case IO_LCDC: // lcdc
		{
			ppu.ppu_write();

			const u8 lcdc_old = io[IO_LCDC];

			io[IO_LCDC] = v;

			// lcd switched off this write
			if(!is_set(v,7) && is_set(lcdc_old,7)) 
			{
				ppu.turn_lcd_off();
			}
			
			// lcdc turned on this write
			else if(is_set(v,7) && !is_set(lcdc_old,7))
			{
				ppu.turn_lcd_on();
			}


			if(!is_set(v,5) && is_set(lcdc_old,5))
			{
				// disable the window
				ppu.window_disable();
			}

			break;
		}


		// lcd stat
		// writes can trigger interrupts on dmg?
		case IO_STAT:
		{
			ppu.ppu_write();

			// delete writeable bits
			io[IO_STAT] &= 7;
				
			// set the w bits with the value
			io[IO_STAT] = io[IO_STAT] |  (v & ~7);
			
			// set unused bit
			io[IO_STAT] |= 0x80;
			ppu.write_stat();
			break;
		}

		case IO_SCX:
		{
			ppu.ppu_write();
			io[IO_SCX] = v;
			break;
		}


		case IO_SCY:
		{
			ppu.ppu_write();
			io[IO_SCY] = v;
			break;
		}


		case IO_WX:
		{
			ppu.ppu_write();
			io[IO_WX] = v;
			break;
		}


		case IO_WY:
		{
			ppu.ppu_write();
			io[IO_WY] = v;
			break;
		}

		case IO_BGP:
		{
			ppu.ppu_write();
			io[IO_BGP] = v;
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
			ppu.stat_update();
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
			if(cpu.is_cgb)
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
				page_table[0xd] = &cgb_wram_bank[cgb_wram_bank_idx][0];
			}
			
			else
			{
				io[IO_SVBK] = 0xff;
			}
			
			break;
		}

		case IO_SPEED:
		{
			// not cgb return ff 
			if(!cpu.is_cgb)
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
			// not cgb 
			if(!cpu.is_cgb)
			{
				io[IO_VBANK] = 0xff;
			}
			
			else
			{
				vram_bank = v & 1;
				io[IO_VBANK] = v | 254;
				if(ppu.get_mode() != ppu_mode::pixel_transfer)
				{
					unlock_vram();
				}
			}
			break;
		}

		// change wether sprites sorted by oam order or x cord
		case IO_OPRI: 
		{
			if(cpu.is_cgb)
			{
				io[IO_OPRI] = v & 1;
			}
			break;
		}


		case IO_BGPI:
		{
			// not cgb return ff 
			if(cpu.is_cgb)
			{
				ppu.set_bg_pal_idx(v);
				io[IO_BGPI] = v | 0x40;
			}
			break;
		}

		case IO_BGPD: // finish later 
		{
			if(cpu.is_cgb)
			{
				ppu.write_bgpd(v);
			}
			break;
		}

		case IO_SPPI: // sprite pallete index
		{

			if(cpu.is_cgb)
			{
				ppu.set_sp_pal_idx(v);
				io[IO_SPPI] = v | 0x40;
			}
			break;
		}		
		
		case IO_SPPD: // sprite pallete data
		{
			if(cpu.is_cgb)
			{		
				ppu.write_sppd(v);
			}
			break;
		}
	
		// specifies src byte dest of dma
		case IO_HDMA1:
		{
			if(cpu.is_cgb)
			{
				dma_src &= 0xff;
				dma_src |= v << 8;
				io[IO_HDMA1] = v;
			}
			break;
		}
		
		// lo byte dma src
		case IO_HDMA2:
		{
			if(cpu.is_cgb)
			{
				v &= 0xf0;
				dma_src &= ~0xff;
				dma_src |= v;
				io[IO_HDMA2] = v;
			}
			break;
		}
		
		
		// high byte dma dst
		case IO_HDMA3:
		{
			if(cpu.is_cgb)
			{
				v &= 0x1f;
				dma_dst &= 0xff;
				dma_dst |= v << 8;
				io[IO_HDMA3] = v;
			}
			break;
		}
		
		// low byte dma dst
		case IO_HDMA4:
		{
			if(cpu.is_cgb)
			{
				v &= 0xf0;
				dma_dst &= ~0xff;
				dma_dst |= v;
				io[IO_HDMA4] = v;
			}
			break;
		}
	
		// cgb dma start
		case IO_HDMA5: // <-- unsure on this behavior
		{
			if(cpu.is_cgb)
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
			}
			break;
		}

		// infared communications
		case IO_RP:
		{	
			// stubbed
			if(cpu.is_cgb)
			{
				io[IO_RP] = v;
			}
			break;
		}

		// sound registers
		case IO_NR10:
		{
			apu.psg.write_nr10(v);
			break;
		}



		case IO_NR11:
		{
			apu.psg.write_nr11(v);
			break;
		}

		case IO_NR12:
		{
			apu.psg.write_nr12(v);
			break;
		}

		case IO_NR13:
		{
			apu.psg.write_nr13(v);
			break;
		}

		case IO_NR14:
		{
			apu.psg.write_nr14(v);
			// trigger causes freq reload update the scheduler
			if(is_set(v,7))
			{
				apu.insert_chan1_period_event();
			}
			break;
		}

		case IO_NR21:
		{
			apu.psg.write_nr21(v);
			break;
		}

		case IO_NR22:
		{
			apu.psg.write_nr22(v);
			break;
		}

		case IO_NR23:
		{
			apu.psg.write_nr23(v);
			break;
		}

		case IO_NR24:
		{
			apu.psg.write_nr24(v);
			// trigger causes freq reload update the scheduler
			if(is_set(v,7))
			{
				apu.insert_chan2_period_event();
			}
			break;
		}


		case IO_NR30:
		{
			apu.psg.write_nr30(v);
			break;
		}


		case IO_NR31:
		{
			apu.psg.write_nr31(v);
			break;
		}

		case IO_NR32:
		{
			apu.psg.write_nr32(v);
			break;
		}

		case IO_NR33:
		{
			apu.psg.write_nr33(v);
			break;
		}

		case IO_NR34:
		{
			apu.psg.write_nr34(v);
			// trigger causes freq reload update the scheduler
			if(is_set(v,7))
			{
				apu.insert_chan3_period_event();
			}
			break;
		}

		case IO_NR41:
		{
			apu.psg.write_nr41(v);
			break;
		}

		case IO_NR42:
		{
			apu.psg.write_nr42(v);
			break;
		}

		case IO_NR43:
		{
			apu.psg.write_nr43(v);

			// incase we havent done any events due the scheduler being off reinsert noise
			if(!scheduler.is_active(gameboy_event::c4_period_elapse))
			{
				noise_reload_period(apu.psg.channels[3],apu.psg.noise);
				apu.insert_chan4_period_event();
			}
			break;
		}

		case IO_NR44:
		{
			apu.psg.write_nr44(v);
			break;
		}		


		// nr 50
		case IO_NR50:
		{
			apu.psg.write_nr50(v);
			break;
		}

			
		case IO_NR51:
		{
			apu.psg.write_nr51(v);
			break;
		}



		// bits 0-3 read only 7 r/w 4-6 unused
		case IO_NR52:
		{
			const auto nr52 = apu.psg.read_nr52();
			// if we have disabled sound we should
			// zero all the registers (nr10-nr51) 
			// and lock writes until its on
			if(is_set(nr52,7) && !is_set(v,7))
			{
				apu.disable_sound();
			}
			
			// enabled
			else if(!is_set(nr52,7) && is_set(v,7))
			{
				apu.enable_sound();
			}
			break;
		}


		// wave table
		case 0x30: case 0x31: case 0x32: case 0x33:
		case 0x34: case 0x35: case 0x36: case 0x37:
		case 0x38: case 0x39: case 0x3a: case 0x3b:
		case 0x3c: case 0x3d: case 0x3e: case 0x3f:
		{
			apu.psg.write_wave_table(addr-0xff30,v);
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
	const u16 source = dma_src & 0xfff0;
	
	const u16 dest = (dma_dst & 0x1ff0) | 0x8000;
	
	// hdma5 stores how many 16 byte incremnts we have to transfer
	const int len = ((io[IO_HDMA5] & 0x7f) + 1) * 0x10;

	// 8 t cycles to xfer one 0x10 block
	// (need to verify the timings on this!)

	for(int i = 0; i < len; i += 2)
	{
		write_mem(dest+i,read_mem(source+i));
		write_mem(dest+i+1,read_mem(source+i+1));
		cpu.cycle_tick_t(1);
	}

	io[IO_HDMA5] = 0xff; // terminate the transfer
}


void Memory::do_hdma() noexcept
{

	if(!hdma_active)
	{
		return;
	}

	const u16 source = (dma_src & 0xfff0) + hdma_len_ticked*0x10;

	const u16 dest = ((dma_dst & 0x1ff0) | 0x8000) + hdma_len_ticked*0x10;

	/*if(!(source <= 0x7ff0 || ( source >= 0xa000 && source <= 0xdff0)))
	{
		printf("ILEGGAL HDMA SOURCE: %X!\n",source);
		exit(1);
	}
	*/

	// 8 t cycles to xfer one 0x10 block

	// find out how many cycles we tick but for now just copy the whole damb thing 						
	for(int i = 0; i < 0x10; i += 2)
	{
		write_mem(dest+i,read_mem(source+i));
		write_mem(dest+i+1,read_mem(source+i+1));
		cpu.cycle_tick_t(1);
	}
	
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


#ifdef DEBUG
void Memory::write_iot_debug(u16 addr, u8 v) noexcept
{
	if(debug.breakpoint_hit(addr,v,break_type::write))
	{
		// halt until told otherwhise :)
		write_log(debug,"[DEBUG] write breakpoint hit ({:x}:{:})",addr,v);
		debug.halt();
	}

	write_iot_no_debug(addr,v);	
}
#endif

void Memory::write_iot_no_debug(u16 addr,u8 v) noexcept
{
	scheduler.service_events();
    write_io(addr,v);
	cpu.cycle_tick(1); // tick for mem access
}

// wram zero bank 0xc000 - 0xd000
void Memory::write_wram_low(u16 addr,u8 v) noexcept
{
    wram[addr&0xfff] = v;
}

// banked wram 0xd000 - 0xe000
// also at 0xe000 - 0xfe00 in echo ram
void Memory::write_wram_high(u16 addr,u8 v) noexcept
{
    cgb_wram_bank[cgb_wram_bank_idx][addr&0xfff] = v;
}

// high ram 0xf000
// we bundle io into this but the hram section is at 0xff80-ffff
void Memory::write_hram(u16 addr,u8 v) noexcept
{
	scheduler.service_events();
    // io regs
    if(addr >= 0xff00)
    {
        write_io(addr,v);
    }

    // high wram mirror
    else if(addr >= 0xf000 && addr <= 0xfdff)
    {
        std::invoke(memory_table[0xd].write_memf,this,addr,v);
    }

	// oam is accesible during mode 0-1
	else if(addr >= 0xfe00 && addr <= 0xfeff)
	{
		write_oam(addr,v);
		return;
	}


    else // restricted
    {

    }
}

void Memory::frame_end()
{
	if(cart_ram_dirty)
	{
		if(++frame_count >= FRAME_SAVE_LIMIT)
		{
			save_cart_ram();
			frame_count = 0;
			cart_ram_dirty = false;
		}
	}
}

void Memory::write_cart_ram(u16 addr, u8 v) noexcept
{
    if(enable_ram && cart_ram_bank != CART_RAM_BANK_INVALID)
    {
        cart_ram_banks[cart_ram_bank][addr & 0x1fff] = v;
		cart_ram_dirty = true;
    }
}

// only bottom 4 bits are readable
void Memory::write_cart_ram_mbc2(u16 addr, u8 v) noexcept
{
    if(enable_ram) // fixed for 512by4 bits
    {
        cart_ram_banks[0][addr & 0x1ff] = ((v & 0xf) | 0xf0);
		cart_ram_dirty = true;
    }
}


}