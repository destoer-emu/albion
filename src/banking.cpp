#include "headers/memory.h"


// 0x0000 - 0x1fff
void Memory::ram_bank_enable(uint16_t address, uint8_t v) 
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


// MBC1

// 0x2000 - 0x3fff

// mbc1 low rom bank change sets lower 5 bits of bank index
void Memory::change_lo_rom_bank_mbc1(uint16_t address, uint8_t v) 
{
	UNUSED(address);
	uint8_t lower5 = v & 31; // get lower 5 bits 
	cart_rom_bank &= 224; // turn off bits lower than 5
	cart_rom_bank |= lower5;
	
	// if the current rom bank its 0,0x20,0x40,0x60 set it to one 
	if(cart_rom_bank == 0 || cart_rom_bank == 0x20
		|| cart_rom_bank == 0x40 || cart_rom_bank == 0x60) 
    {
		
		cart_rom_bank += 1;
	}
	
	// bank greater than the current number of rom banks wraps back round
	if(cart_rom_bank >= rom_info.no_rom_banks) 
    {
		cart_rom_bank %= rom_info.no_rom_banks;
	}

}

// 0x4000 - 0x5fff

// changes the kind of bank switch mbc1 does
void Memory::mbc1_banking_change(uint16_t address, uint8_t v) 
{
	UNUSED(address);
	
	if(rom_banking) 
    {
		change_hi_rom_bank_mbc1(v);
	} 
	
	else 
    {
		ram_bank_change_mbc1(v);
	}
}

// 0x6000 - 0x7fff
void Memory::change_mode_mbc1(uint16_t address, uint8_t v) 
{
	UNUSED(address);
	
	rom_banking = (v & 0x1);
	
	// only access banking zero in rom banking mode
	if(rom_banking) 
    {
		cart_ram_bank = 0;
	}
}

// a well programmed game should not cause this to be called 
// but many will
void Memory::banking_unused(uint16_t address, uint8_t v) 
{
	UNUSED(address); UNUSED(v);
	return;
}


// mbc1 mode banking funcs
void Memory::change_hi_rom_bank_mbc1(uint8_t v) 
{
	cart_rom_bank &= 0x1f;
	cart_rom_bank |= (v & 0xe0);
	
	// if the current rom bank its 0,0x20,0x40,0x60 set it to one 
	if(cart_rom_bank == 0 || cart_rom_bank == 0x20
		|| cart_rom_bank == 0x40 || cart_rom_bank == 0x60) 
    {
		
		cart_rom_bank += 1;
	}	

	// bank greater than the current number of rom banks wraps back round
	if(cart_rom_bank >= rom_info.no_rom_banks) 
	{
		cart_rom_bank %= rom_info.no_rom_banks;
	}

}


void Memory::ram_bank_change_mbc1(uint8_t v) 
{
	cart_ram_bank = v & 0x3; //  max 3 banks in mbc1
	
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
//0x2000 - 0x4000
void Memory::change_lo_rom_bank_mbc2(uint16_t address,uint8_t data)
{
	UNUSED(address);
	cart_rom_bank = data & 0xf;
	if(cart_rom_bank == 0) 
	{
		cart_rom_bank = 1;
	}
	return;
}

// 0x0000 - 0x2000
void Memory::ram_bank_enable_mbc2(uint16_t address,uint8_t v)
{
	UNUSED(address);

	if(is_set(address,4)) // dont enabel if bit 4 of address written to is set
	{
		return;
	}

	ram_bank_enable(address,v);
}



// mbc3

// mbc3 ( lower 7 bits of rom bank index set here)
// 0x2000 - 0x4000
void Memory::change_rom_bank_mbc3(uint16_t address,uint8_t v)
{
	UNUSED(address);
	cart_rom_bank = v & 127;
	cart_rom_bank &= 127;
			
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
void Memory::mbc3_ram_bank_change(uint16_t address,uint8_t v)
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

// 0x2000 - 0x4000
void Memory::change_lo_rom_bank_mbc5(uint16_t address,uint8_t data)
{
	UNUSED(address);
	cart_rom_bank &= 0x100;
	cart_rom_bank |= data;
				
	if(cart_rom_bank >= rom_info.no_rom_banks)
	{
		cart_rom_bank %= rom_info.no_rom_banks;

	}			
	// bank zero actually acceses bank 0
}


//mbc5 (9th bit) (03000 - 0x3fff)
void Memory::change_hi_rom_bank_mbc5(uint16_t address,uint8_t data)
{
	UNUSED(address);
	cart_rom_bank &= 0xff;
	cart_rom_bank |= (data & 1) << 8; // 9th bank bit
	if(cart_rom_bank >= rom_info.no_rom_banks)
	{
		cart_rom_bank %= rom_info.no_rom_banks;
	}
}

// 0x4000 - 0x6000
// mbc5
void Memory::mbc5_ram_bank_change(uint16_t address,uint8_t data)
{
	UNUSED(address);
	cart_ram_bank = data & 0xf;
	
	if(cart_ram_bank >= rom_info.no_ram_banks)
	{
		cart_ram_bank %= rom_info.no_ram_banks;	
	}	
}