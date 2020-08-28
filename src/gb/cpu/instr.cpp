#include<gb/gb.h>
 
namespace gameboy
{

void Cpu::set_zero(uint8_t reg) noexcept
{
    zero = !reg;
}


// decrement
void Cpu::instr_dec(uint8_t reg) noexcept
{
    reg -= 1;

    // the N flag
	negative = true;

    set_zero(reg);

	// check the carry 
	half = is_set(((reg+1)&0xf)-1,4);
}

void Cpu::instr_inc(uint8_t reg) noexcept
{
	// deset negative
	negative = false;

	reg += 1;
	
    set_zero(reg);

	// test carry from bit 3
	// set the half carry if there is
	half = is_set(((reg-1)&0xf) + 1,4);
}


void Cpu::instr_bit(uint8_t reg, uint8_t bit) noexcept
{
	// unuset negative
	negative = false;

	zero = !is_set(reg,bit);
	
	// set half carry
	half = true;	
}


// logical and  and n with a
void Cpu::instr_and(uint8_t num) noexcept
{
	// set only the half carry flag
	half = true;
	carry = false;
	negative = false;

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
	reg = carry? set_bit(reg,0) : deset_bit(reg,0);
	
	// deset half carry 
	half = false;
	negative = false;
	
	// Carry flag gets bit 7 of reg
	carry = cond;


	set_zero(reg);
	
	return reg;
}


void Cpu::instr_sub(uint8_t num) noexcept
{
	// set negative
	negative = true;
	
	zero = a == num;


	// check half carry
	half = (((a & 0x0f) - (num & 0x0f)) < 0);

	carry = num > a;

	a -= num;
}

// compare (same as sub ignore result)
void Cpu::instr_cp(uint8_t num) noexcept
{

	negative = true;
	
	zero = a == num;

	// check half carry
	half = (((a & 0x0f) - (num & 0x0f)) < 0);


	carry = num > a; 

}


void Cpu::instr_sbc(uint8_t num) noexcept
{	
	const uint8_t reg = a;

	const int carry_val = carry ? 1 : 0;
	
	const int result = reg - num - carry_val;
	
	// set negative
	negative = true;
	
	carry = (result < 0);

	
	half = ((reg & 0x0f) - (num & 0x0f) - carry_val < 0);

	a = result;
	set_zero(a);
}


void Cpu::instr_add(uint8_t num) noexcept
{
	// deset negative
	negative = false;


	// test carry from bit 3
	// set the half carry if there is
	half = (is_set((a & 0x0f) + (num & 0x0f),4));

	
	// check carry from bit 7
	carry = (a + num > 255);
		
	a += num;
	set_zero(a);
}


// for the sp add opcodes
uint16_t Cpu::instr_addi(uint16_t reg, int8_t num) noexcept
{
	// deset negative & zero
	negative = false;
	zero = false;

	// test carry from bit 3
	// set the half carry if there is
	half = (is_set((reg & 0x0f) + (num & 0x0f),4));
	
	carry = (is_set((reg & 0xff) + (num & 0xff),8));

	
	reg += num;	
	return reg;
}

// add n + carry flag to a 
void Cpu::instr_adc(uint8_t num)  noexcept
{
	const uint8_t reg = a;
	
	const int carry_val = carry ? 1 : 0;
	
	const int result = reg + num + carry_val;
	
	// deset negative
	negative = false;
	
	carry = (result > 0xff);

	
	half = (is_set((reg & 0x0f) + (num & 0x0f) + carry_val,4));
		
	a = result;
	set_zero(a);
}


uint16_t Cpu::instr_addw(uint16_t reg, uint16_t num) noexcept 
{
	// deset negative
	negative = false;

	// check for carry from bit 11
	half = (is_set((reg & 0x0fff) + (num & 0x0fff),12));
	
	// check for full carry 
	carry = (is_set(reg + num,16));

	reg += num;
	
	return reg;
}

void Cpu::instr_or(uint8_t val) noexcept
{
	a |= val;
	// reset flags
	negative = false;
	half = false;
	carry = false;
	set_zero(a);
}

// swap upper and lower nibbles 
uint8_t Cpu::instr_swap(uint8_t num) noexcept
{
	// reset flags
	negative = false;
	half = false;
	carry = false;

    set_zero(num);

	return ((num & 0x0f) << 4 | (num & 0xf0) >> 4);
}

void Cpu::instr_xor(uint8_t num) noexcept
{
	// reset flags
	negative = false;
	half = false;
	carry = false;

	a ^= num;
	set_zero(a);
}




// shift left into carry deset bit 1
uint8_t Cpu::instr_sla(uint8_t reg) noexcept
{
	// reset flags
	half = false;
	negative = false;

	bool cond = is_set(reg,7); // cache if 7 bit is set

	reg <<= 1;
	
	// deset bit one
	reg = deset_bit(reg,0);

	set_zero(reg);
	
	carry = cond;

	return reg;
}


// shift left into carry deset bit 7
uint8_t Cpu::instr_sra(uint8_t reg) noexcept
{
	negative = false;
	half = false;
	
	const bool cond = is_set(reg,0);// cache if 0 bit is set
	const bool set = is_set(reg,7);
	
	reg >>= 1;
	
	if(set)
	{
		reg = set_bit(reg,7);
	}
	
	carry = cond;
	
	set_zero(reg);
	
	return reg;
}

// can probably remove duplication here lol 

uint8_t Cpu::instr_srl(uint8_t reg)  noexcept 
{
	half = false;
	negative = false;

	carry = is_set(reg,0);

	
	reg >>= 1;

	set_zero(reg);
	
	return reg;
}





uint8_t Cpu::instr_rr(uint8_t reg) noexcept
{
	const bool set = is_set(reg,0);
	
	reg >>= 1;
	
	// bit 7 gets carry 
	reg = carry? set_bit(reg,7) : deset_bit(reg,7);

	// deset negative
	negative = false;
	// unset half
	half = false;
	
	// carry gets bit 0
	carry = set;

	set_zero(reg);
	
	return reg;
}	



uint8_t Cpu::instr_rrc(uint8_t reg) noexcept
{
	carry = is_set(reg,0);
	
	negative = false;
	half = false;

	reg >>= 1;
	
	if(carry)
	{
		reg = set_bit(reg,7);
	}
	set_zero(reg);
	return reg;
}


uint8_t Cpu::instr_rlc(uint8_t reg) noexcept
{
	carry = is_set(reg,7);
		
	reg <<= 1;
	
	negative = false;
	half = false;
	
	if(carry)
	{
		reg = set_bit(reg,0);
	}	

	set_zero(reg);
	return reg;
}


void Cpu::instr_jr() noexcept
{
	const auto operand = static_cast<int8_t>(mem.read_memt(pc++));
	cycle_delay(4); // internal delay
	pc += operand;	
}

void Cpu::instr_jr_cond(bool cond, bool flag) noexcept
{
	const auto operand = static_cast<int8_t>(mem.read_memt(pc++));
	if(flag == cond)
	{
		cycle_delay(4); // internal delay
		pc += operand;
	}	
}


void Cpu::instr_jp_cond(bool cond, bool flag) noexcept
{
	uint16_t v =  mem.read_wordt(pc);
	pc += 2;
	if(cond == flag)
	{
		pc = v;
		cycle_delay(4); // internal delay
	}	
}

void Cpu::call_cond(bool cond, bool flag) noexcept
{
	const uint16_t v = mem.read_wordt(pc);
	pc += 2;
	if(flag == cond)
	{
		cycle_delay(4);  // internal delay
		write_stackwt(pc);
		pc = v;
	}
}


void Cpu::ret_cond(bool cond, bool flag) noexcept
{
	cycle_delay(4); // internal
	if(flag == cond)
	{
		pc = read_stackwt();
		cycle_delay(4);  // internal
	}	
}

}