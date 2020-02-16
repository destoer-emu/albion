#include <gb/cpu.h>
#include <gb/memory.h>
#include <gb/disass.h>
#include <destoer-emu/debug.h>
#include <gb/apu.h>


namespace gameboy
{


void Cpu::check_rst_loop(uint16_t addr, uint8_t op)
{
	if(mem->read_mem(addr) == op)
	{
		write_log("[ERROR] rst infinite loop at {:x}->{:x}",pc,addr);
		throw std::runtime_error("infinite rst lockup");
	}
}

void Cpu::exec_instr()
{

#ifdef DEBUG
	uint8_t x = mem->read_mem(pc);
	if(debug->step_instr || debug->breakpoint_hit(pc,x,break_type::execute))
	{
		// halt until told otherwhise :)
		write_log("[DEBUG] execute breakpoint hit ({:x}:{:x})",pc,x);
		debug->halt();
	}
#endif


    uint8_t opcode = mem->read_memt(pc++);

	// pc fails to increment if halt bug happens
	if(halt_bug)
	{
		halt_bug = false;
		pc--;
	}


    switch(opcode)
    {
		case 0x0: // nop
			break;
		
		case 0x1: // ld bc, nn
			write_bc(mem->read_wordt(pc));
			pc += 2;
			break;
		
		case 0x2: // ld (bc), a
			mem->write_memt(read_bc(),a);
			break;
		
		case 0x3: // inc bc
			write_bc(read_bc()+1);
			cycle_tick(1); // internal
			break;
		
		case 0x4: // inc b
			instr_inc(b++);
			break;
			
		case 0x5: // dec b
			instr_dec(b--);
			break;
			
		case 0x6: // ld b, n
			b = mem->read_memt(pc++);
			break;
			
		case 0x7: // rlca (rotate a left bit 7 to carry)
			a = instr_rlc(a);
			f = deset_bit(f,Z);
			break;
		
		
		case 0x8: // ld (nnnn), sp
			mem->write_wordt(mem->read_wordt(pc),sp);
			pc += 2; // for two immediate ops
			break;
		
		case 0x9: // add hl,bc
			write_hl(instr_addw(read_hl(),read_bc()));
			cycle_tick(1); // internal
			break;
			

		
		case 0xa: // ld a, (bc)
			a = mem->read_memt(read_bc());
			break;
		
		
		case 0xb: // dec bc 
			write_bc(read_bc()-1);
			cycle_tick(1); // internal
			break;
		
		
		
		case 0xc: // inc c
			instr_inc(c++);
			break;
		
		case 0xd: // dec c
			instr_dec(c--);
			break;
		
		
		case 0xe: // ld c, nn
			c = mem->read_memt(pc++);
			break;

			
		case 0xf: // rrca
			a = instr_rrc(a);
			f = deset_bit(f,Z);
			break;
			
			// most games should never even execute this 
		case 0x10: // stop 
			pc += 1; // skip over next byte
			
			if(is_cgb && is_set(mem->io[IO_SPEED],0))
			{
				mem->io[IO_SPEED] = deset_bit(mem->io[IO_SPEED],0); // clear the bit
				is_double = !is_double;
				
				if(is_double)
				{
					mem->io[IO_SPEED] = set_bit(mem->io[IO_SPEED],7);
				}
			
				else // single speed 
				{
					mem->io[IO_SPEED] = deset_bit(mem->io[IO_SPEED],7);
				}
				apu->set_double(is_double);
			}
			
			else // almost nothing triggers this 
			{
				write_log("[WARNING] stop opcode hit at {:x}",pc);
			}

			break;
			
		case 0x11: // ld de, nn
			write_de(mem->read_wordt(pc));
			pc += 2;
			break;
		
		case 0x12: // ld (de), a
			mem->write_memt(read_de(), a);
			break;
		
		case 0x13: // inc de
			write_de(read_de()+1);
			cycle_tick(1); // internal
			break;
		
		case 0x14: // inc d
			instr_inc(d++);
			break;
		
		case 0x15: // dec d
			instr_dec(d--);
			break;
		
		case 0x16: // ld d, nn 
			d = mem->read_memt(pc++);
			break;
		
		case 0x17: // rla (rotate left through carry flag) 
			a = instr_rl(a);
			f = deset_bit(f,Z);
			break;
		
		case 0x18: // jr n
			instr_jr();
			break;
		
		case 0x19: // add hl, de
			write_hl(instr_addw(read_hl(),read_de()));
			cycle_tick(1); // internal
			break;
		
		case 0x1a: // ld a,(de) 
			a = mem->read_memt(read_de());
			break;
		

		case 0x1b: // dec de
			write_de(read_de()-1);
			cycle_tick(1); // internal
			break;
			
		case 0x1c: // inc e
			instr_inc(e++);
			break;
		
		case 0x1d: // dec e
			instr_dec(e--);
			break;
			
		case 0x1e: // ld e, n
			e = mem->read_memt(pc++);
			break;
		
		case 0x1f: // rra
			a = instr_rr(a);
			f = deset_bit(f,Z);
			break;
		
		case 0x20: // jr nz, n
			instr_jr_cond(false,Z);
			break;
			
		case 0x21: // ld hl, nn
			write_hl(mem->read_wordt(pc));
			pc += 2;
			break;
		
		case 0x22: // ldi (hl), a
			mem->write_memt(read_hl(),a);
			write_hl(read_hl()+1);
			break;
		
		case 0x23: // inc hl
			write_hl(read_hl()+1); // increment hl
			cycle_tick(1); // internal
			break;
		
		case 0x24: // inc h
			instr_inc(h++);
			break;
		
		case 0x25: // dec h
			instr_dec(h--);
			break;
		
		case 0x26: // ld h, nn
			h = mem->read_memt(pc++);
			break;
		
		case 0x27: // daa (lots of edge cases)
			//https://forums.nesdev.com/viewtopic.php?f=20&t=15944
			if (!is_set(f,N)) 
            {  
				// after an addition, adjust if (half-)carry occurred or if result is out of bounds
				if (is_set(f,C)|| a > 0x99) 
                { 
					a += 0x60; 
                    f = set_bit(f,C); 
				}
				if (is_set(f,H) || (a & 0x0f) > 0x09)  
                { 
					a += 0x6; 
				}
			} 
			
			else 
            {  
				// after a subtraction, only adjust if (half-)carry occurred
				if (is_set(f,C)) 
                {
					a -= 0x60; 
				}
				
				if (is_set(f,H)) 
                { 
					a -= 0x6; 
				}
			}
			
			// preserve C and N flags
			f &= (1 << C) | (1 << N);

			set_zero(a);
			break;
			
		case 0x28: // jr z, n
			instr_jr_cond(true,Z);
			break;
		
		case 0x29: // add hl, hl
			write_hl(instr_addw(read_hl(),read_hl()));
			cycle_tick(1); // internal
			break;
		
		// flags affected by this?
		case 0x2a: // ldi a, (hl)
			
			a = mem->read_memt(read_hl());
			write_hl(read_hl()+1);
			break;
		
		case 0x2b: // dec hl
			write_hl(read_hl()-1);
			cycle_tick(1); // internal
			break;
		
		case 0x2c: // inc l
			instr_inc(l++);
			break;
		
		case 0x2d: // dec l
			instr_dec(l--);
			break;
		
		case 0x2e: // ld l, nn
			l = mem->read_memt(pc++);
			break;
			
		case 0x2f: // cpl (flip bits in a)
			// set H and N
			f = set_bit(f,N);
			f = set_bit(f,H);
			a = ~a;
			break;
		
		case 0x30: // jr nc, nn
			instr_jr_cond(false,C);
			break;
		
		case 0x31: // ld sp, nn
			sp = mem->read_wordt(pc);
			pc += 2;
			break;
		
		case 0x32: // ldd (hl), a 
			mem->write_memt(read_hl(),a);
			write_hl(read_hl()-1);
			break;
		
		case 0x33: // inc sp
			sp += 1;
			cycle_tick(1); // internal
			break;
		
		case 0x34: // inc (hl)
        {
			uint8_t v = mem->read_memt(read_hl()); // use to store (hl)
			instr_inc(v++); // inc 
			mem->write_memt(read_hl(),v); // and write back
			break;
        }

		case 0x35: // dec (hl)
        {
			uint8_t v = mem->read_memt(read_hl());
			instr_dec(v--); // dec it
			mem->write_memt(read_hl(),v); // and write straight back	
			break;
        }	
		
		case 0x36: // ld (hl), n 
			mem->write_memt(read_hl(),mem->read_memt(pc++));
			break;
		
		case 0x37: // scf
			// set the carry flag deset h and N
			f = set_bit(f,C);
			f = deset_bit(f,N);
			f = deset_bit(f,H);
			break;
		
		case 0x38: // jr c, nnnn
			instr_jr_cond(true,C);
			break;
			
		case 0x39: // add hl, sp 
			write_hl(instr_addw(read_hl(),sp));
			cycle_tick(1); // internal
			break;	
			
		case 0x3a: // ldd a, (hl)
			a = mem->read_memt(read_hl());
			write_hl(read_hl()-1);
			break;
		
		case 0x3b: // dec sp
			sp -= 1;
			cycle_tick(1); // internal
			break;
		
		case 0x3c: // inc a
			instr_inc(a++);
			break;
		
		case 0x3d: // dec a
			instr_dec(a--);
			break;
			
		
		case 0x3e: // ld a, n
			a = mem->read_memt(pc++);
			break;
		
		case 0x3f: // ccf
			if(is_set(f,C))
			{ // complement the carry flag (probably a neat way to do this)
				f = deset_bit(f,C);
			}
			
			else
			{
				f = set_bit(f,C);
			}
			
			f = deset_bit(f,N);
			f = deset_bit(f,H);
			break;
		
		case 0x40: // ld b, b
			// do nothing lol
			break;
		
		case 0x41: // ld b, c
			b = c;
			break;
		
		case 0x42: // ld b, d
			b = d;
			break;
		
		case 0x43: // ld b, e
			b = e;
			break;
		
		case 0x44: // ld b, h
			b = h;
			break;
		
		case 0x45: // ld b, l
			b = l;
			break;
		
		case 0x46: // ld b, (hl)
			b = mem->read_memt(read_hl());
			break;
		
		case 0x47: // ld b,a
			b = a;
			break;
		
		case 0x48: // ld c, b 
			c = b;
			break;
		
		case 0x49: // ld c, c 
			// nop
			break;
		
		case 0x4a: // ld c, d 
			c = d;
			break;
		
		case 0x4b: // ld c, e 
			c = e;
			break;
		
		case 0x4c: // ld c, h
			c = h;
			break;
		
		case 0x4d: // ld c ,l 
			c = l;
			break;
		
		case 0x4e: // ld c, (hl)
			c = mem->read_memt(read_hl());
			break;
		
		case 0x4f: // ld c,a
			c = a;
			break;
		
		
		case 0x50: // ld d, b 
			d = b;
			break;
		
		case 0x51: // ld d, c 
			d = c;
			break;
		
		
		case 0x52: // ld d, d
			// nop lol 
			break;
		
		
		case 0x53: // ld d, e
			d = e;
			break;
		
		case 0x54: // ld d, h
			d = h;
			break;
		
		case 0x55: // ld d , l 
			d = l;
			break;
		
		case 0x56: // ld d, (hl)
			d = mem->read_memt(read_hl());
			break;
		
		case 0x57: // ld d, a
			d = a;
			break;
		
		
		case 0x58: // ld e, b 
			e = b;
			break;
		
		case 0x59: // ld e, c
			e = c;
			break;
		
		case 0x5a: // ld e, d
			e = d; 
			break;
		
		case 0x5b: // ld e, e
			// nop 
			break;
		
		case 0x5c: // ld e,h
			e = h;
			break;
			
		case 0x5d: // ld e, l
			e = l;
			break;
			
		case 0x5e: // ld e, (hl)
			e = mem->read_memt(read_hl());
			break;
		
		case 0x5f: // ld e, a
			e = a;
			break;
		
		
		case 0x60: // ld h, b
			h = b;
			break;
		
		case 0x61: // ld h, c
			h = c;
			break;
		
		case 0x62: // ld h, d
			h = d;
			break;
		
		
		case 0x63: // ld h, e
			h = e;
			break;
			
		case 0x64: // ld h, h
			// nop;
			break;
			
		case 0x65: // ld h, l 	
			h = l;
			break;
			
		case 0x66: // ld h, (hl)
			h = mem->read_memt(read_hl());
			break;
		
		case 0x67: // ld h, a 
			h = a;
			break;
		
		case 0x68: // ld l, b
			l = b;
			break;
		
		case 0x69: // ld l,c
			l = c;
			break;
		
		
		case 0x6a: // ld l, d
			l = d;
			break;
		
		case 0x6b: // ld l, e
			l = e;
			break;
		
		case 0x6c: // ld l, h 
			l = h;
			break;
			
		case 0x6d: // ld l, l
			// nop
			break;
		
		case 0x6e: // ld l, (hl)
			
			l = mem->read_memt(read_hl());
			
			break;
		
		case 0x6f: // ld l, a
			l = a;
			break;
		
		case 0x70: // ld (hl),b
			mem->write_memt(read_hl(),b);
			break;
		
		case 0x71: // ld (hl), c
			mem->write_memt(read_hl(), c);
			break;
		
		case 0x72: // ld (hl), d
			mem->write_memt(read_hl(),d);
			break;
		
		case 0x73: // ld (hl), e
			mem->write_memt(read_hl(),e);
			break;
		
		case 0x74: // ld (hl), h
			mem->write_memt(read_hl(),h);
			break;
		
		case 0x75: // ld (hl), l
			mem->write_memt(read_hl(),l);
			break;
		
		case 0x76: // halt 
			// caller will handle
			halt = true;
			break;
		
		case 0x77: // ld (hl), a 
			mem->write_memt(read_hl(),a);
			break;
		
		case 0x78: // ld a, b
			a = b;
			break;
		
		case 0x79: //ld a, c
			a = c;
			break;
		
		case 0x7a: // ld a, d
			a = d;
			break;
		
		case 0x7b: // ld a, e
			a = e;
			break;
		
		case 0x7c: // ld a, h
			a = h;
			break;
		
		case 0x7d: // ld a, l
			a = l;
			break;
		
		case 0x7e: // ld a, (hl)
			a = mem->read_memt(read_hl());
			break;
		
		case 0x7f: // ld a, a
			// nop 
			break;
		
		case 0x80: // add b
			instr_add(b);
			break;
		
		case 0x81: // add c
			instr_add(c);
			break;
		
		case 0x82: // add d
			instr_add(d);
			break;
		
		case 0x83: // add e
			instr_add(e);
			break;
		
		case 0x84: // add h
			instr_add(h);
			break;
		
		case 0x85: // add l
			instr_add(l);
			break;
		
		case 0x86: // add a, (hl)
        {
		    uint8_t v = mem->read_memt(read_hl());
			instr_add(v);
			break;
        }

		case 0x87: // add a
			instr_add(a);
			break;
		
		case 0x88: // adc a, b
			instr_adc(b);
			break;
		
		case 0x89: // adc c (add carry + n)
			instr_adc(c);
			break;
		
		case 0x8a: // adc d
			instr_adc(d);
			break;
			
		case 0x8b: // adc e
			instr_adc(e);
			break;
			
		case 0x8c: // adc h
			instr_adc(h);
			break;
			
		case 0x8d: // adc l
			instr_adc(l);
			break;
		
		case 0x8e: // adc (hl)
        {
			uint8_t v = mem->read_memt(read_hl());
			instr_adc(v);
			break;
        }

		case 0x8f: // adc a
			instr_adc(a);
			break;
		
		case 0x90: // sub b
			instr_sub(b);
			break;
		
		case 0x91: // sub c
			instr_sub(c);
			break;
		
		case 0x92: // sub d
			instr_sub(d);
			break;
			
		case 0x93: // sub e
			instr_sub(e);
			break;
			
		case 0x94: // sub h
			instr_sub(h);
			break;
			
		case 0x95: // sub l
			instr_sub(l);
			break;
		
		case 0x96: // sub (hl)
        {
			uint8_t v = mem->read_memt(read_hl());
			instr_sub(v);
			break;
        }

		case 0x97: // sub a 
			instr_sub(a);
			break;
		
		case 0x98: // sbc, a, b
			instr_sbc(b);
			break;
			
		case 0x99: // sbc a, c
			instr_sbc(c);
			break;
			
		case 0x9a: // sbc a ,d
			instr_sbc(d);
			break;
			
		case 0x9b: // sbc a, e
			instr_sbc(e);
			break;
			
		case 0x9c: // sbc a, h 
			instr_sbc(h);
			break;
			
		case 0x9d: // sbc a, l
			instr_sbc(l);
			break;
		
		case 0x9e: // sbc a, (hl)
        {
			uint8_t v = mem->read_memt(read_hl());
			instr_sbc(v);
			break;
        }

		case 0x9f: // sbc a, a
			instr_sbc(a);
			break;
		
		case 0xa0: // and b
			instr_and(b);
			break;
		
		case 0xa1: // and c
			instr_and(c);
			break;
		
		case 0xa2: // and d
			instr_and(d);
			break;
		
		case 0xa3: // and e
			instr_and(e);
			break;
		
		case 0xa4: // and h
			instr_and(h);
			break;
			
		case 0xa5: // and l
			instr_and(l);
			break;
		
		case 0xa6: // and (hl)
			instr_and(mem->read_memt(read_hl()));
			break;
		
		case 0xa7: // and a
			instr_and(a);
			break;
		

		case 0xa8: // xor b
			instr_xor(b);
			break;
		
		case 0xa9: // xor c 
			instr_xor(c);
			break;
		
		case 0xaa: // xor d
			instr_xor(d);
			break;
			
		case 0xab: // xor e
			instr_xor(e);
			break;
			
		case 0xac: // xor h
			instr_xor(h);
			break;
		
		case 0xad: // xor l
			instr_xor(l);
			break;
		
		case 0xae: // xor (hl)
        {
			uint8_t v = mem->read_memt(read_hl());
			instr_xor(v);
			break;
        }

		// could shortcut case end up with just zero flag being set 
		case 0xaf: // xor a, a
			instr_xor(a);
			break;
		
		case 0xb0: // or b
			instr_or(b);
			break;
		
		case 0xb1: // or c (a is implicit)
			instr_or(c);
			break;
		
		case 0xb2: // or d
			instr_or(d);
			break;
		
		case 0xb3: // or e
			instr_or(e);
			break;
			
		case 0xb4: // or h
			instr_or(h);
			break;
			
		case 0xb5: // or l
			instr_or(l);
			break;
		
		case 0xb6: // or (hl)
			instr_or(mem->read_memt(read_hl()));
			break;
		
		case 0xb7: // or a
			//instr_or(a);
			// a | a = a 
			// only thing that can happen is the zero flag setting
			f = 0; // clear flags
			set_zero(a);
			break;
		
		case 0xb8: // cp b (sub but ignore result only keep flags)
			instr_cp(b);
			break;

		case 0xb9: // cp c
			instr_cp(c);
			break;
		
		case 0xba: // cp d
			instr_cp(d);
			break;
		
		case 0xbb: // cp e
			instr_cp(e);
			break;
		
		case 0xbc: // cp h
			instr_cp(h);
			break;
			
		case 0xbd: // cp l
			instr_cp(l);
			break;
		
		case 0xbe: // cp (hl)
        {
			uint8_t v = mem->read_memt(read_hl());
			instr_cp(v);
			break;
        }

		case 0xbf: // cp a <-- probably can be optimised to a constant
			instr_cp(a);
			break;
		
		case 0xc0: // ret nz
			ret_cond(false,Z);
			break;
	
		case 0xc1: // pop bc	
			write_bc(read_stackwt());	
			break;
		
		case 0xc2: // jp nz, nnnn
        {
			uint16_t v =  mem->read_wordt(pc);
			pc += 2;
			if(!is_set(f,Z))
			{
				pc = v;
				cycle_tick(1); // internal delay
			}
			break;
        }

		case 0xc3: // jump
			pc = mem->read_wordt(pc);
			cycle_tick(1); // internal
			break;
		
		
		case 0xc4: // call nz
        {
			call_cond(false,Z);
			break;
        }

		case 0xc5: // push bc 	
			cycle_tick(1); // internal
			write_stackwt(read_bc());
			break;
		
		
		case 0xc6: // add a, nn
			instr_add(mem->read_memt(pc++));
			break;
		
		case 0xc7: // rst 00
			check_rst_loop(0x00,0xc7);
			cycle_tick(1); // internal
			write_stackwt(pc);
			pc = 0;
			break;
		
		case 0xc8: // ret z
        {
			ret_cond(true,Z);
			break;
        }

		case 0xc9: // ret 
			pc = read_stackwt();	
			cycle_tick(1); // internal
			break;
		
		case 0xca: // jp z, nnnn
        {
			uint16_t v = mem->read_wordt(pc);
			pc += 2;
			if(is_set(f, Z))
			{
				pc = v;
				cycle_tick(1); // internal
			}
			break;
        }

		case 0xcb: // multi len opcode (cb prefix)
        {
			uint8_t cbop = mem->read_memt(pc++); // fetch the opcode
			// tick our instr fetch for cb
			exec_cb(cbop); // exec it
			break; 
        }
		
		case 0xcc: // call z
        {
			call_cond(true,Z);
			break;
        }
		case 0xCD: // call nn 
        {
			uint16_t v = mem->read_wordt(pc);
			pc += 2;
			cycle_tick(1); // internal
			write_stackwt(pc);
			pc = v;
			break;
        }

		case 0xce: // adc a, nn
			instr_adc(mem->read_memt(pc++));
			break;
		
		case 0xcf: // rst 08
			check_rst_loop(0x08,0xcf);	
			cycle_tick(1); // internal
			write_stackwt(pc);
			pc = 0x8;
			break;
		
		case 0xd0: // ret nc
			ret_cond(false,C);
			break;
		
		case 0xd1: // pop de
			write_de(read_stackwt());
			break;
		
		case 0xd2: // jp nc u16
        {
			uint16_t v = mem->read_wordt(pc);
			pc += 2;
			if(!is_set(f,C))
			{
				pc = v;
				cycle_tick(1);// internal
			}
			break;
        }

		case 0xd4: // call nc nnnn
        {
			call_cond(false,C);
			break;			
        }

		case 0xD5: // push de
			cycle_tick(1); // internal delay 
			write_stackwt(read_de());
			break;
		
		case 0xd6: // sub a, nn
			instr_sub(mem->read_memt(pc++));
			break;
		
		case 0xd7: // rst 10
			#ifdef DEBUG
			check_rst_loop(0x10,0xd7);
			#endif
			cycle_tick(1); // internal
			write_stackwt(pc);
			pc = 0x10;
			break;
		
		case 0xd8: // ret c
			ret_cond(true,C);
			break;
			
		case 0xd9: // reti
			pc = read_stackwt();	
			cycle_tick(1);// internal
			interrupt_enable = true; // re-enable interrupts
			break;
		
		case 0xda: // jp c, u16
        {
			uint16_t v = mem->read_wordt(pc);
			pc += 2;
			if(is_set(f,C))
			{
				pc = v;
				cycle_tick(1); // internal
			}
			break;
        }

		case 0xdc: // call c, u16
        {
			call_cond(true,C);
			break;
        }

		case 0xde: // sbc a, n
			instr_sbc(mem->read_memt(pc++));
			break;

		case 0xdf: // rst 18
			check_rst_loop(0x18,0xdf);
			cycle_tick(1); // internal
			write_stackwt(pc);
			pc = 0x18;
			break;
		
		case 0xE0: // ld (ff00+n),a
			mem->write_iot((0xff00+mem->read_memt(pc++)),a);
			break;

		case 0xe1: // pop hl
			write_hl(read_stackwt());
			break;
			
		case 0xE2: // LD ($FF00+C),A
			mem->write_iot(0xff00 + c, a);
			break;

		case 0xe5: // push hl
			cycle_tick(1); // internal
			write_stackwt( read_hl());
			break;
		
		case 0xe6: // and a, n
			instr_and(mem->read_memt(pc++));
			break;
		
		case 0xe7: // rst 20
			check_rst_loop(0x20,0xe7);
			cycle_tick(1); // internal
			write_stackwt(pc);
			pc = 0x20;
			break;
		
		case 0xe8: // add sp, i8 
			sp = instr_addi(sp, static_cast<int8_t>(mem->read_memt(pc++)));
			cycle_tick(2); // internal delay (unsure)
			break;
		
		case 0xe9: // jp hl
			pc = read_hl();
			break;
		
		case 0xea: // ld (nnnn), a
			mem->write_memt(mem->read_wordt(pc),a);
			pc += 2;
			break;
		
		case 0xee: // xor a, nn
			instr_xor(mem->read_memt(pc++));
			break;
		
		case 0xef: // rst 28
			check_rst_loop(0x28,0xef);
			cycle_tick(1); // internal
			write_stackwt(pc);
			pc = 0x28;
			break;
		
		case 0xF0: // ld a, (ff00+nn)
			a = mem->read_iot(0xff00+mem->read_memt(pc++));
			break;
		
		case 0xf1: // pop af
			write_af(read_stackwt());
			break;
		
		case 0xf2: // ld a, (ff00+c)
			a = mem->read_iot(0xff00 + c);
			break;
		
		case 0xf3: // disable interrupt
			// needs to be executed after the next instr
			// main routine will handle
			di = true;
			break;
		
		case 0xf5: // push af
			cycle_tick(1); // internal delay
			write_stackwt(read_af());
			break;
		
		case 0xf6: // or a, nn
			instr_or(mem->read_memt(pc++));
			break;
		
		case 0xf7: // rst 30
			check_rst_loop(0x30,0xf7);
			cycle_tick(1); // internal
			write_stackwt(pc);
			pc = 0x30;
			break;
		
		case 0xf8: // ld hl, sp + i8 
			write_hl(instr_addi(sp,static_cast<int8_t>(mem->read_memt(pc++))));
			cycle_tick(1); // internal
			break;
		
		case 0xf9: // ld sp, hl
			sp = read_hl();
			cycle_tick(1); // internal
			break;
		
		case 0xfa: // ld a (nn) <-- 16 bit address
			a = mem->read_memt(mem->read_wordt(pc));
			pc += 2;
			break;
		
		case 0xfb: 
			// caller will check opcode and handle it
			ei = true;
			break;
		
		case 0xFE: // cp a, nn (do a sub and discard result)
			instr_cp(mem->read_memt(pc++));
			break;
			
		
		case 0xff: // rst 38
			check_rst_loop(0x38,0xff);	
			cycle_tick(1); // internal 
			write_stackwt(pc);
			pc = 0x38;
			break;   

		default:
		{
			write_log("[ERROR] invalid opcode at {:x}",pc);
			throw std::runtime_error("invalid opcode!");		
		}
    }
}

}