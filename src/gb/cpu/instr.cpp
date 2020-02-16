#include <gb/cpu.h>
#include <gb/memory.h>
 
namespace gameboy
{

void Cpu::set_zero(uint8_t reg) noexcept
{
    if(!reg)
    {
        f = set_bit(f,Z);
    }
}


// decrement
void Cpu::instr_dec(uint8_t reg) noexcept
{
    reg -= 1;

    // preserve carry
    f &= (1 << C);

    // the N flag
    f = set_bit(f,N);

    set_zero(reg);


	// check the carry 
	if(!(((((reg+1) & 0xf) + (-1 & 0xf)) & 0x10) == 0x10))
	{
		f = set_bit(f,H);
	}
}

void Cpu::instr_inc(uint8_t reg) noexcept
{
	reg += 1;
	
	// preserve carry
	f &= (1 << C);
	
    set_zero(reg);

	// test carry from bit 3
	// set the half carry if there is
	if(((((reg-1)&0xf) + (1&0xf))&0x10) == 0x10)
	{
		f = set_bit(f,H);
	}
}


void Cpu::instr_bit(uint8_t reg, uint8_t bit) noexcept
{
	f &= (1 << C); // preserve Carry
	if(!is_set(reg,bit)) // if bit set
	{
		f = set_bit(f,Z);	
	}
	
	// set half carry
	f = set_bit(f,H);	
}


// logical and  and n with a
void Cpu::instr_and(uint8_t num) noexcept
{
	// set only the half carry flag
	f = set_bit(0,H);

	// set if result is zero 
	a &= num;
	set_zero(a);
}


// the register's bits are shifted left. 
// The carry value is put into 0th bit of the register, 
// and the leaving 7th bit is put into the carry.

uint8_t Cpu::instr_rl(uint8_t reg) noexcept
{
	const bool cond = is_set(reg,7); // cache if 7 bit is set
	
	// perform the rotation
	// shift the register left
	reg <<= 1;
	
	// bit 0 gets bit of carry flag
	reg |= (f & (1 << C)) >> C; // bit 0 of reg gains carry bit
	
	// deset neagtive and half carry 
	f = 0;	
	
	// Carry flag gets bit 7 of reg
	if(cond)
	{
		f = set_bit(f,C);
	}

	set_zero(reg);
	
	return reg;
}


void Cpu::instr_sub(uint8_t num) noexcept
{
	// set only the negative flag
	f = set_bit(0,N);
	
	if(a == num)
	{
		f = set_bit(f,Z);
	}

	// check half carry
	if((( static_cast<int>((a & 0xf)) - static_cast<int>((num & 0xf)) ) < 0))
	{
		f = set_bit(f,H);
	}

	if(num > a)
	{
		f = set_bit(f,C); // set the carry
	}

	a -= num;
}

// compare (same as sub ignore result)
void Cpu::instr_cp(uint8_t num) noexcept
{
	// set only the negative flag
	f = set_bit(0,N);
	
	if(a == num)
	{
		f = set_bit(f,Z);
	}
	
	// check half carry
	if((( static_cast<int>((a & 0xf)) - static_cast<int>((num & 0xf)) ) < 0))
	{
		f = set_bit(f,H);
	}

	if(num > a) 
	{
		f = set_bit(f,C); // set the carry
	}
}


void Cpu::instr_sbc(uint8_t num) noexcept
{	
	const uint8_t reg = a;

	const int carry = is_set(f,C) ? 1 : 0;
	
	const int result = reg - num - carry;
	
	// clear all flags
	f = 0;
	f = set_bit(f,N); // set negative
	
	if(result < 0)
	{
		f = set_bit(f,C);
	}
	
	if((reg & 0x0f) - (num & 0x0f) - carry < 0)
	{
		f = set_bit(f,H);
	}
	
	a = result;
	set_zero(a);
}


void Cpu::instr_add(uint8_t num) noexcept
{
	// reset every flag 
	f = 0;
	
	// test carry from bit 3
	// set the half carry if there is
	if((((a&0xf) + (num&0xf))&0x10) == 0x10)
	{
		f = set_bit(f,H);
	}
	
	// check carry from bit 7
	if(a + num > 255)
	{
		f = set_bit(f, C);
	}
		
	a += num;
	set_zero(a);
}


// for the sp add opcodes
uint16_t Cpu::instr_addi(uint16_t reg, int8_t num) noexcept
{
	f = 0; // reset flags
	// test carry from bit 3
	// set the half carry if there is
	if( (( static_cast<int>((reg&0xf)) + static_cast<int>((num&0xf))) & 0x10) == 0x10)
	{
		f = set_bit(f,H);
	}
	
	if( (( static_cast<int>((reg & 0xff)) + static_cast<int>((num&0xff)) ) & 0x100) == 0x100)
	{
		f = set_bit(f,C);
	}	
	
	reg += num;	
	return reg;
}

// add n + carry flag to a 
void Cpu::instr_adc(uint8_t num)  noexcept
{
	const uint8_t reg = a;
	
	const int carry = is_set(f,C) ? 1 : 0;
	
	const int result = reg + num + carry;
	
	f = 0; // reset all flags
	
	if(result > 0xff)
	{
		f = set_bit(f,C);
	}	
	
	if((reg & 0x0f) + (num & 0x0f) + carry > 0x0f)
	{
		f = set_bit(f,H);
	}
		
	a = result;
	set_zero(a);
}


uint16_t Cpu::instr_addw(uint16_t reg, uint16_t num) noexcept 
{
	f &= (1 << Z); // only preserve the Z flag 

	// check for carry from bit 11
	if((((reg & 0x0fff) + (num&0x0fff)) & 0x1000) == 0x1000)
	{
		f = set_bit(f, H);
	}
	
	// check for full carry 
	if(reg + num > 0xffff)
	{
		f = set_bit(f,C);
	}
	
	reg += num;
	
	return reg;
}

void Cpu::instr_or(uint8_t val) noexcept
{
	a |= val;
	f = 0; // reset all flags
	set_zero(a);
}

// swap upper and lower nibbles 
uint8_t Cpu::instr_swap(uint8_t num) noexcept
{
	// reset flags register
	f = 0;
	
    set_zero(num);

	return ((num & 0x0f) << 4 | (num & 0xf0) >> 4);
}

void Cpu::instr_xor(uint8_t num) noexcept
{
	// reset flags register
	f = 0;
	a ^= num;
	set_zero(a);
}




// shift left into carry deset bit 1
uint8_t Cpu::instr_sla(uint8_t reg) noexcept
{
	f = 0;
	bool cond = is_set(reg,7); // cache if 7 bit is set

	reg <<= 1;
	
	// deset bit one
	reg = deset_bit(reg,0);

	set_zero(reg);
	
	if(cond)
	{
		f = set_bit(f,C);
	}
		
	return reg;
}


// shift left into carry deset bit 7
uint8_t Cpu::instr_sra(uint8_t reg) noexcept
{
	f = 0;
	
	const bool cond = is_set(reg,0);// cache if 0 bit is set
	const bool set = is_set(reg,7);
	
	reg >>= 1;
	
	if(set)
	{
		reg = set_bit(reg,7);
	}
	
	if(cond)
	{
		f = set_bit(f,C);
	}
	
	set_zero(reg);
	
	return reg;
}

// can probably remove duplication here lol 

uint8_t Cpu::instr_srl(uint8_t reg)  noexcept 
{
	f = 0;
	if(is_set(reg,0))
	{
		f = set_bit(f, C);
	}
	
	reg >>= 1;

	set_zero(reg);
	
	return reg;
}





uint8_t Cpu::instr_rr(uint8_t reg) noexcept
{
	const bool set = is_set(reg,0);
	
	reg >>= 1;
	
	
	// bit 7 gets carry 
	if(is_set(f,C))
	{
		reg = set_bit(reg, 7);
	}
	
	else 
	{
		reg = deset_bit(reg,7);
	}
	
	f = 0;
	
	// carry gets bit 0
	if(set)
	{
		f = set_bit(f,C);
	}
	
	set_zero(reg);
	
	return reg;
}	



uint8_t Cpu::instr_rrc(uint8_t reg) noexcept
{
	const bool set = is_set(reg,0);
	
	f = 0;

	reg >>= 1;
	
	if(set)
	{
		f = set_bit(f,C);
		reg = set_bit(reg,7);
	}
	set_zero(reg);
	return reg;
}


uint8_t Cpu::instr_rlc(uint8_t reg) noexcept
{
	const bool set = is_set(reg,7);
		
	reg <<= 1;
	
	f = 0;
	
	if(set)
	{
		f = set_bit(f,C);
		reg = set_bit(reg,0);
	}	
	set_zero(reg);
	return reg;
}


void Cpu::instr_jr() noexcept
{
	const auto operand = static_cast<int8_t>(mem->read_memt(pc++));
	cycle_tick(1); // internal delay
	pc += operand;	
}

void Cpu::instr_jr_cond(bool cond, int bit) noexcept
{
	const auto operand = static_cast<int8_t>(mem->read_memt(pc++));
	if(is_set(f, bit) == cond)
	{
		cycle_tick(1); // internal delay
		pc += operand;
	}	
}

void Cpu::call_cond(bool cond, int bit) noexcept
{
	const uint16_t v = mem->read_wordt(pc);
	pc += 2;
	if(is_set(f,bit) == cond)
	{
		cycle_tick(1);  // internal delay
		write_stackwt(pc);
		pc = v;
	}
}


void Cpu::ret_cond(bool cond, int bit) noexcept
{
	cycle_tick(1); // internal
	if(is_set(f,bit) == cond)
	{
		pc = read_stackwt();
		cycle_tick(1);  // internal
	}	
}

}