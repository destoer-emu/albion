#include <gb/gb.h>

namespace gameboy
{

// 0x0000 - 0x1fff
void Memory::ram_bank_enable(uint16_t address, uint8_t v) noexcept 
{
	UNUSED(address);
	// no ram banks present dont enable them
	if(!rom_info.no_ram_banks) 
    {
		return;
	}
	
	// if data is = 0xa enable ram
	enable_ram = ((v & 0xf) == 0xa);	
}

// whole 8 bits matter for mbc5
void Memory::ram_bank_enable_mbc5(uint16_t address, uint8_t v) noexcept 
{
	UNUSED(address);
	// no ram banks present dont enable them
	if(!rom_info.no_ram_banks) 
    {
		return;
	}
	
	// if data is = 0xa enable ram
	enable_ram = (v == 0xa);	
}


// MBC1

// 0x2000 - 0x3fff

// mbc1 low rom bank change sets lower 5 bits of bank index
void Memory::change_lo_rom_bank_mbc1(uint16_t address, uint8_t v) noexcept 
{
	UNUSED(address);
	const uint8_t data = ((v & 0x1f) == 0) ? 1 : (v & 0x1f);
	cart_rom_bank = (cart_rom_bank & ~0x1f) | data;
	
	// bank greater than the current number of rom banks wraps back round
	if(cart_rom_bank >= rom_info.no_rom_banks) 
    {
		cart_rom_bank %= rom_info.no_rom_banks;
	}
}

// 0x4000 - 0x5fff

// write bank two register
void Memory::mbc1_banking_change(uint16_t address, uint8_t v) noexcept 
{
	UNUSED(address);
	
	mbc1_bank2 = (v & 3);

	// doesent matter what the mode is we allways use the bank2 reg
	// for the higher part of our rom in the 0x4000 - 0x8000 area
	change_hi_rom_bank_mbc1();
	
	if(!rom_banking)
    {
		ram_bank_change_mbc1();
	}
}

// 0x6000 - 0x7fff
void Memory::change_mode_mbc1(uint16_t address, uint8_t v) noexcept
{
	UNUSED(address);
	
	rom_banking = !is_set(v,0);
	
	// only access banking zero in rom banking mode
	if(rom_banking) 
    {
		cart_ram_bank = 0;
	}

	else
	{
		ram_bank_change_mbc1();
	}
}

// a well programmed game should not cause this to be called 
// but many will
void Memory::banking_unused(uint16_t address, uint8_t v) noexcept 
{
	UNUSED(address); UNUSED(v);
	return;
}


// mbc1 mode banking funcs
void Memory::change_hi_rom_bank_mbc1() noexcept
{
	cart_rom_bank = (cart_rom_bank & 0x1f) | (mbc1_bank2 << 5);
	
	// bank greater than the current number of rom banks wraps back round
	if(cart_rom_bank >= rom_info.no_rom_banks) 
	{
		cart_rom_bank %= rom_info.no_rom_banks;
	}
}


void Memory::ram_bank_change_mbc1() noexcept 
{
	cart_ram_bank = mbc1_bank2;

	if(rom_info.no_ram_banks <= 1) 
	{
		cart_ram_bank = 0;
	}
	
	// bank greater than the current number of ram banks wraps back round
	else if(cart_ram_bank >= rom_info.no_ram_banks) 
    {
		cart_ram_bank %= rom_info.no_ram_banks;
	}
}





// mbc2


//mbc2
// this is the outlier and both registers are accessible
// on alternating on the 9th bit being set of addr
//0x0000 - 0x4000
void Memory::lower_bank_write_mbc2(uint16_t address, uint8_t v) noexcept
{
	// ram bank enable
	// 
	if(!is_set(address,8)) 
	{
		ram_bank_enable(address,v);
	}

	else // not set rom bank
	{
		cart_rom_bank = ((v & 0xf) == 0)? 1 : v & 0xf;
		if(cart_rom_bank >= rom_info.no_rom_banks)
		{
			cart_rom_bank %= rom_info.no_rom_banks;
		}	
	}
}



// mbc3

// mbc3 ( lower 7 bits of rom bank index set here)
// 0x2000 - 0x4000
void Memory::change_rom_bank_mbc3(uint16_t address,uint8_t v) noexcept
{
	UNUSED(address);
	cart_rom_bank = v & 127;
	
	if(cart_rom_bank >= rom_info.no_rom_banks)
	{
		cart_rom_bank %= rom_info.no_rom_banks;
	}

	if(cart_rom_bank == 0)
	{
		cart_rom_bank = 1;
	}
}

// 0x4000 - 0x6000
void Memory::mbc3_ram_bank_change(uint16_t address,uint8_t v) noexcept
{
	UNUSED(address);
	// change the ram bank
	// if ram bank is greater than 0x3 disable writes
	cart_ram_bank = v;
			
	if(cart_ram_bank > 3)
	{
		// should signal rtc being accessed
		// but for now leave unimpl
		cart_ram_bank = CART_RAM_BANK_INVALID;
	}	
	
	else if(rom_info.no_ram_banks == 0)
	{
		cart_ram_bank = 0;
	}
	
	else if(cart_ram_bank <= 3 && cart_ram_bank >= rom_info.no_ram_banks)
	{
		cart_ram_bank %= rom_info.no_ram_banks;
	}
}	


// mbc5

// 0x2000 - 0x3000 (lower 8 bits)
void Memory::change_lo_rom_bank_mbc5(uint16_t address,uint8_t data) noexcept
{
	UNUSED(address);
	cart_rom_bank = (cart_rom_bank & ~0xff) | data;
			
	if(cart_rom_bank >= rom_info.no_rom_banks)
	{
		cart_rom_bank %= rom_info.no_rom_banks;
	}			
	// bank zero actually acceses bank 0
}


//mbc5 (9th bit) (03000 - 0x3fff)
void Memory::change_hi_rom_bank_mbc5(uint16_t address,uint8_t data) noexcept
{
	UNUSED(address);
	cart_rom_bank = (cart_rom_bank & 0xff) |  (data & 1) << 8;
	if(cart_rom_bank >= rom_info.no_rom_banks)
	{
		cart_rom_bank %= rom_info.no_rom_banks;
	}
}

// 0x4000 - 0x6000
// mbc5
void Memory::mbc5_ram_bank_change(uint16_t address,uint8_t data) noexcept
{
	UNUSED(address);
	cart_ram_bank = data & 0xf;
	
	if(cart_ram_bank >= rom_info.no_ram_banks)
	{
		cart_ram_bank %= rom_info.no_ram_banks;	
	}	
}

}