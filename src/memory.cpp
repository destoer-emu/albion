#include "headers/memory.h"
#include "headers/ppu.h"
#include "headers/cpu.h"
#include "headers/lib.h"


bool Memory::is_lcd_enabled()
{
	return ( is_set(io[IO_LCDC],7) );	
}

void Memory::init(Cpu *c,Ppu *p,std::string rom_name)
{
    cpu = c; // init our cpu pointer
    ppu = p;

    read_file(rom_name,rom); // read our rom in


    // propagate an error back with an exception later for now we just bail
    if(rom.size() < 0x4000)
    {
        std::cout << "Rom is too small!";
        throw std::runtime_error("rom is too small!");
    }


    // reserve our underlying memory
    vram.resize(0x2);
    for(auto &x: vram)
    {
        x.resize(0x2000);
    }


    wram.resize(0x1000);
    oam.resize(0xa0);
    io.resize(0x100);


    // pull out our rom info
    rom_info.init(rom);


    cart_ram_banks.resize(rom_info.no_ram_banks);
    for(auto &x: cart_ram_banks)
    {
        x.resize(0x2000);
    }




    cgb_wram_bank.resize(7);
    for(auto &x: cgb_wram_bank)
    {
        x.resize(0x1000);
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

    // banking
    if(rom_info.mbc1)
    {
		memory_table[0x0].write_memf = &Memory::ram_bank_enable;
		memory_table[0x1].write_memf = &Memory::ram_bank_enable;
		memory_table[0x2].write_memf = &Memory::change_lo_rom_bank_mbc1;
		memory_table[0x3].write_memf = &Memory::change_lo_rom_bank_mbc1;
		memory_table[0x4].write_memf = &Memory::mbc1_banking_change;
		memory_table[0x5].write_memf = &Memory::mbc1_banking_change;
		memory_table[0x6].write_memf = &Memory::change_mode_mbc1;
		memory_table[0x7].write_memf = &Memory::change_mode_mbc1;        
    }

    // rom only
    else if(rom_info.cart_type == 0)
    {
        for(int i = 0; i < 8; i++)
        {
            memory_table[i].write_memf = &Memory::banking_unused;
        }
    }
    
	else
	{
		throw std::runtime_error("unknown banking type!");
	}

	cgb_wram_bank_idx = 0;
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
}




// public access functions
uint8_t Memory::read_mem(uint16_t addr)
{
    return std::invoke(memory_table[(addr & 0xf000) >> 12].read_memf,this,addr);
}

void Memory::write_mem(uint16_t addr, uint8_t v)
{
    return std::invoke(memory_table[(addr & 0xf000) >> 12].write_memf,this,addr,v);    
}


uint16_t Memory::read_word(uint16_t addr)
{
    return read_mem(addr) | (read_mem(addr+1) << 8);
}


void Memory::write_word(uint16_t addr, uint16_t v)
{
    write_mem(addr+1,(v&0xff00)>>8);
    write_mem(addr,(v&0x00ff));
}


// memory accesses (timed)
uint8_t Memory::read_memt(uint16_t addr)
{
    uint8_t v = read_mem(addr);
    cpu->cycle_tick(1); // tick for the memory access 
    return v;
}

void Memory::write_memt(uint16_t addr, uint8_t v)
{
    write_mem(addr,v);
    cpu->cycle_tick(1); // tick for the memory access    
}


uint16_t Memory::read_wordt(uint16_t addr)
{
    return read_memt(addr) | (read_memt(addr+1) << 8);
}


void Memory::write_wordt(uint16_t addr, uint16_t v)
{
    write_memt(addr+1,(v&0xff00)>>8);
    write_memt(addr,(v&0x00ff));
}


// read mem underyling
// object attribute map 0xfe00 - 0xfe9f
uint8_t Memory::read_oam(uint16_t addr)
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
uint8_t Memory::read_vram(uint16_t addr)
{
    // vram is used in pixel transfer cannot access
    if(ppu->mode != ppu_mode::pixel_transfer)
    {
        addr -= 0x8000;
        return vram[vram_bank][addr];
    }

    else
    {
        return 0xff;
    }
}

// cart  memory 0xa000 - 0xc000
uint8_t Memory::read_cart_ram(uint16_t addr)
{ 
    if(enable_ram && cart_ram_bank != CART_RAM_BANK_INVALID)
    {
        addr -= 0xa000;
        return cart_ram_banks[cart_ram_bank][addr];
    }

    else
    {
        return 0xff;
    }
}

// 0xff00 io regs (has side affects)
uint8_t Memory::read_io(uint16_t addr)
{
    switch(addr & 0xff)
    {

        // joypad control reg <-- used for sgb command packets too
		case IO_JOYPAD:
		{		
			// read from mem
			uint8_t req = io[IO_JOYPAD];
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

        default: 
        {
            return io[addr & 0xff];
        }
    }
}

uint8_t Memory::read_iot(uint16_t addr)
{
    uint8_t v = read_io(addr);
    cpu->cycle_tick(1); // tick for mem access
    return v;
}

// for now we will just return the rom
// 0x4000 - 0x8000 return current rom bank
uint8_t Memory::read_rom_bank(uint16_t addr)
{   
    addr -= 0x4000;
    return rom[(cart_rom_bank*0x4000)+addr];
}


//0x0000 - 0x4000 return rom bank zero
uint8_t Memory::read_bank_zero(uint16_t addr)
{
    return rom[addr];
}

// 0xc000 && 0xe000 (echo ram)
// return wram non banked
uint8_t Memory::read_wram_low(uint16_t addr)
{
    return wram[addr&0xfff];
}

// 0xd000
// return banked wram this is fixed on dmg
uint8_t Memory::read_wram_high(uint16_t addr)
{
    return cgb_wram_bank[cgb_wram_bank_idx][addr&0xfff];
}

// 0xf000 various
uint8_t Memory::read_hram(uint16_t addr)
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
void Memory::write_oam(uint16_t addr,uint8_t v)
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
void Memory::write_vram(uint16_t addr,uint8_t v)
{
    // vram is used in pixel transfer cannot access
    if(ppu->mode != ppu_mode::pixel_transfer)
    {
        addr -= 0x8000;
        vram[vram_bank][addr] = v;
    }
}


void Memory::do_dma(uint8_t v)
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
			write_oam(0xfe00+i, read_mem(dma_address+i)); 	
		}
		oam_dma_active = true; // indicate a dma is active and to lock memory
		oam_dma_address = dma_address; // the source address
		oam_dma_index = 0; // how far along the dma transfer we are		
	}
}

// this needs work
void Memory::tick_dma(int cycles)
{
    // not active do nothing!
    if(!oam_dma_active) 
    {
        return;
    }


	if(oam_dma_index > 0xa0)
	{
		oam_dma_index += cycles;
		if(oam_dma_index >= 0xa2) // now its done 
		{
			oam_dma_active = false;
			return;
		}	
	}

	
	for(int i = 0; i < cycles; i++)
	{
		oam_dma_index += 1; // go to next one
		// We are done with our dma
		if(oam_dma_index > 0xa0) 
		{ 			
			// get ready to add cycles as 2 extra are used for start / stop
			cycles -= i;
			break;
		}
	}
	
	if(oam_dma_index > 0xa0)
	{
		oam_dma_index += cycles;
		if(oam_dma_index >= 0xa2) // now its done 
		{
			oam_dma_active = false;
			return;
		}	
	}
}


// io memory has side affects 0xff00
void Memory::write_io(uint16_t addr,uint8_t v)
{
    switch(addr & 0xff)
    {

		// update the timer freq (tac in gb docs)
		case IO_TMC:
		{
			io[IO_TMC] = v | 248;
			return;
		}		

		// div and tima share the same internal counter
		// should account for this internal behavior
		case IO_DIV:
		{
			cpu->internal_timer = 0;
			return;
		}
			
		case IO_IF:
		{
			io[IO_IF] = v | (128 + 64 + 32); // top 3 bits allways on
			return;
		}

		case IO_LCDC: // lcdc
		{
			if(!is_set(v,7) && is_lcd_enabled()) // lcd switched off this write
			{
				ppu->set_scanline_counter(0); // counter is reset
				ppu->current_line = 0; // reset ly
				io[IO_STAT] &= ~3; // mode 0
				ppu->mode = ppu_mode::hblank;
			}
			
			if(is_set(v,7) && !is_lcd_enabled())
			{
				ppu->set_scanline_counter(0);
				io[IO_STAT] |= 2; // mode 2?
				ppu->mode = ppu_mode::oam_search;
			}
			
			io[IO_LCDC] = v;
			return;
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
			return;
		}


		// block ly writes
		case IO_LY:
		{
			return;
		}

		// implement timing on dma and page boundary checking
		case IO_DMA: // dma reg perform a dma transfer //<-- may need seperate handling in a do_dma
		{
			do_dma(v);
			return;
		}


        default: // hram
        {
            io[addr & 0xff] = v;
            return;
        }
    }
}

void Memory::write_iot(uint16_t addr,uint8_t v)
{
    write_io(addr,v);
    cpu->cycle_tick(1); // tick for mem access
}

// wram zero bank 0xc000 - 0xd000
void Memory::write_wram_low(uint16_t addr,uint8_t v)
{
    wram[addr&0xfff] = v;
}

// banked wram 0xd000 - 0xe000
// also at 0xe000 - 0xfe00 in echo ram
void Memory::write_wram_high(uint16_t addr,uint8_t v)
{
    cgb_wram_bank[cgb_wram_bank_idx][addr&0xfff] = v;
}

// high ram 0xf000
// we bundle io into this but the hram section is at 0xff80-ffff
void Memory::write_hram(uint16_t addr,uint8_t v)
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


void Memory::write_cart_ram(uint16_t addr, uint8_t v)
{
    if(enable_ram && cart_ram_bank != CART_RAM_BANK_INVALID)
    {
        addr -= 0xa000;
        cart_ram_banks[cart_ram_bank][addr] = v;
    }
}