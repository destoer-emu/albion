#include <gba/cpu.h>
#include <gba/memory.h>
#include <gba/disass.h>

namespace gameboyadvance
{

// if there is a pipeline stall (whenever pc changes besides a fetch)
void Cpu::arm_fill_pipeline() // need to verify this...
{
    pipeline[0] = mem.read_memt<uint32_t>(regs[PC]);
    regs[PC] += ARM_WORD_SIZE;
    pipeline[1] = mem.read_memt<uint32_t>(regs[PC]);
    regs[PC] += ARM_WORD_SIZE;
}

// fetch opcodes and handle the 3 stage
// pipeline
uint32_t Cpu::fetch_arm_opcode()
{

    // ignore the pipeline for now
    regs[PC] &= ~3; // algin

    uint32_t opcode = mem.read_memt<uint32_t>(regs[PC]);
    regs[PC] += ARM_WORD_SIZE;
    return opcode;
}

void Cpu::execute_arm_opcode(uint32_t instr)
{
    // get the bits that determine the kind of instr it is
    uint32_t op = get_arm_opcode_bits(instr);

    // call the function from our opcode table
    std::invoke(arm_opcode_table[op],this,instr);    
}

void Cpu::exec_arm()
{
    uint32_t instr = fetch_arm_opcode();

    // if the condition is not met just
    // advance past the instr
    if(!cond_met((instr >> 28) & 0xf))
    {
       return;
    }

    execute_arm_opcode(instr);
}




void Cpu::arm_unknown(uint32_t opcode)
{
    uint32_t op = ((opcode >> 4) & 0xf) | ((opcode >> 16) & 0xff0);
    auto err = fmt::format("[cpu-arm {:08x}] unknown opcode {:08x}:{:08x}\n",regs[PC],opcode,op);
    throw std::runtime_error(err);
}


void Cpu::arm_mull(uint32_t opcode)
{
    UNUSED(opcode);
    int rm = opcode & 0xf;
    int rs = (opcode >> 8) & 0xf;
    int rdhi = (opcode >> 16) & 0xf;
    int rdlo = (opcode >> 12) & 0xf;
    bool s = is_set(opcode,20);
    bool a = is_set(opcode,21);
    bool u = !is_set(opcode,22);
    

    uint64_t result;

    if(u) // unsigned
    {
        uint64_t ans;
        if(a)
        {
            uint64_t oper = ((uint64_t)regs[rdhi] << 32) | (uint64_t)regs[rdlo];
            ans = (uint64_t)regs[rs] * (uint64_t)regs[rm] + oper;
            cycle_tick(2); // 1s + 1i
        }

        else
        {
            ans = (uint64_t)regs[rs] * (uint64_t)regs[rm];
            cycle_tick(1); // 1s
        }
        result = ans;       
    }

    else // signed
    {
        int64_t ans;
        int64_t v1 = sign_extend<int64_t>(regs[rs],32);
        int64_t v2 = sign_extend<int64_t>(regs[rm],32);
        if(a)
        {
            int64_t oper = ((int64_t)regs[rdhi] << 32) | (int64_t)regs[rdlo];
            ans =  v1 * v2 + oper;
            cycle_tick(2); // 1s + 1i
        }

        else
        {
            ans = v1 * v2;
            cycle_tick(1); // 1s
        }
        result = (uint64_t)ans;
    }

    // write the ans
    regs[rdhi] = (result >> 32) & 0xffffffff;
    regs[rdlo] = result & 0xffffffff;


    // c & v destroyed?
    if(s)
    {
        set_nz_flag_long(result);

        // c & v destroyed
        cpsr = deset_bit(cpsr,C_BIT);
        cpsr = deset_bit(cpsr,V_BIT);

    }
}

// neeeds a more accurate timings fix
void Cpu::arm_mul(uint32_t opcode)
{
    int rn = (opcode >> 12) & 0xf;
    int rd = (opcode >> 16) & 0xf;
    int rs = (opcode >> 8) & 0xf;
    int rm = opcode & 0xf;
    bool s = is_set(opcode,20);
    bool a = is_set(opcode,21);

    if(a) // mla
    {
        regs[rd] = regs[rm] * regs[rs] + regs[rn];
    }   

    else // mul
    {
        regs[rd] = regs[rm] * regs[rs];
    }

    if(s)
    {
        set_nz_flag(regs[rd]);

        // c destroyed
        cpsr = deset_bit(cpsr,C_BIT);
    }

    cycle_tick(1);
}

void Cpu::arm_swap(uint32_t opcode)
{
    int rm = opcode & 0xf;
    int rd = (opcode >> 12) & 0xf;
    int rn = (opcode >> 16) & 0xf;

    bool is_byte = is_set(opcode,22);

    // swp works propely even if rm and rn are the same
    uint32_t tmp; 

    // rd = [rn], [rn] = rm
    if(is_byte)
    {
        tmp = mem.read_memt<uint8_t>(regs[rn]);
        mem.write_memt<uint8_t>(regs[rn],regs[rm]);
    }

    else
    {
        tmp = mem.read_memt<uint32_t>(regs[rn]);
        regs[rd] = rotr(regs[rd],(regs[rn]&3)*8);
        mem.write_memt<uint32_t>(regs[rn],regs[rm]);
    }


    regs[rd] = tmp;

    // 1s +2n +1i
    cycle_tick(4);

}

// <--- double check this code as its the most likely error source
void Cpu::arm_block_data_transfer(uint32_t opcode)
{
    bool p = is_set(opcode,24);
    bool u = is_set(opcode,23);
    bool s = is_set(opcode,22); // psr or force user mode
    bool w = is_set(opcode,21);
    bool l = is_set(opcode,20);
    int rn = (opcode >> 16) & 0xf;
    int rlist = opcode & 0xffff;



    uint32_t addr = regs[rn];
    uint32_t old_base = regs[rn];
    int n = 0;

   
    int first = 0;
    // do in reverse order so we can pull
    // the first item without doing something jank
    for(int i = 15; i >= 0; i--)
    {
        if(is_set(rlist,i))
        {
            first = i;
            n++;
        }
    }

    bool has_pc = is_set(rlist,PC);

    // allways adding on address so if  we are in "down mode"
    // we need to precalc the buttom1
    if(!u) 
    {
        addr -= n * ARM_WORD_SIZE;
        if(w)
        {
            regs[rn] = addr;
            w = false;
        }
        // invert the pre/post
        // as predoing the addr has messed with the meaning
        p = !p;  
    }


    cpu_mode old_mode = arm_mode;
    bool changed_mode = false;
    if(s)
    {
        // no r15 or load uses different mode
        // dont bother switching if are in user mode
        if((!has_pc || !l) && arm_mode != cpu_mode::user)
        {
            changed_mode = true;
            switch_mode(cpu_mode::user);
        }
    }





    for(int i = first; i < 16; i++)
    {
        if(!is_set(rlist,i))
        {
            continue;
        }

        if(p) 
        {
            addr += ARM_WORD_SIZE;
        }


        if(l) // load
        {
            // no writeback if base
            if(i == rn)
            {
               w = false;
            }
            regs[i] = mem.read_memt<uint32_t>(addr);

            if(i == PC && s) // if pc is in list and s bit set  cpsr = spsr
            {
                int idx = static_cast<int>(arm_mode);
                if(arm_mode < cpu_mode::user)  // not in user or system mode
                {
                    set_cpsr(status_banked[idx]);
                }

                else
                {
                    auto err = fmt::format("[block data: {:08x}] illegal status bank {:x}\n",regs[PC],idx);
                    throw std::runtime_error(err);
                }
            }
        }

        else // store
        {
            // if base is is first entry
            // store old base
            if(rn == i && i == first)
            {
                mem.write_memt<uint32_t>(addr,old_base);
            }

            else
            {
                mem.write_memt<uint32_t>(addr,regs[i]);
            }
        }

        if(!p)
        {
            addr += ARM_WORD_SIZE;
        }
    }

    //writeback higher address if it went up
    if(w)
    {
        regs[rn] = addr;
    }


    if(l)
    {
        if(rn != PC)
        {
            // ns+1n+1i
            cycle_tick(n+2);
        }

        else
        {
            // (n+1)S +2n + 1i
            cycle_tick(n+4);
        }

    }

    else
    {
        //n-1S +2n
        cycle_tick(n+1);
    }


    // restore the correct mode!
    if(changed_mode)
    {
        switch_mode(old_mode);
    }
}


// need to handle instr variants and timings on these

void Cpu::arm_branch(uint32_t opcode)
{
    // account for prefetch operation
    uint32_t pc = regs[PC] + ARM_WORD_SIZE;

    // 24 bit offset is sign extended to 32 bit
    // and shifted left by two
    int32_t offset = sign_extend<int32_t>(opcode & 0xffffff,24) << 2;


    // if the link bit is set this acts as a call instr
    if(is_set(opcode,24))
    {
        // bits 0:1  are allways cleared
        regs[LR] = (regs[PC] & ~3);
        write_log(debug,"[cpu-arm {:08x}] call {:08x}",regs[PC],pc+offset);
    }


    regs[PC] = pc + offset;
    cycle_tick(3); //2s + 1n cycles
}

// psr transfer
void Cpu::arm_psr(uint32_t opcode)
{
    bool is_msr = is_set(opcode,21); // 21 set msr else mrs
    bool spsr = is_set(opcode,22); // to cpsr or spsr?

    bool is_imm = is_set(opcode,25);



    // msr
    if(is_msr)
    {
        // msr mask
        uint32_t mask = 0;

        if(is_set(opcode,19)) mask |= 0xff000000;
        if(is_set(opcode,18)) mask |= 0x00ff0000;
        if(is_set(opcode,17)) mask |= 0x0000ff00;
        if(is_set(opcode,16)) mask |= 0x000000ff;


        if(arm_mode == cpu_mode::user) // only flags can be changed in user mode
        {
            mask = 0xf0000000;
        }


        // either rotr imm or reg rm
        uint32_t v = is_imm? get_arm_operand2_imm(opcode) : regs[opcode & 0xf];

        // only write specifed bits
        v &= mask;
        
        // all bits in mask should be deset
        // and then the value ored
        if(!spsr) // cpsr
        {
            set_cpsr((cpsr & ~mask) | v);
        }

        else // spsr
        {
            int idx = static_cast<int>(arm_mode);
            if(arm_mode < cpu_mode::user)
            {
                status_banked[idx] = (status_banked[idx] & ~mask) | v;
            }

            else // writes ignored (see starbreezes test)
            {

            }
        }
    }

    // mrs
    else
    {
        int rd = (opcode >> 12) & 0xf;

        if(spsr)
        {
            int idx = static_cast<int>(arm_mode);
            if(arm_mode < cpu_mode::user)
            {
                regs[rd] = status_banked[idx];
            }

            else // user and system read cpsr where spsr would normally be read
            {
                regs[rd] = cpsr;
            }
        }

        else
        {
            regs[rd] = cpsr;
        }

    }

    cycle_tick(1); // one s cycle
}

// generalize this so we can include other opcodes


/*flag handling
z and n are easy
how we handle V and C i dont know
what has higher precedence the shifter carry
or that of the operation eg add
and how we will 
a) detect the carry
b) set them appropiately...

// logical ones C is the carry out of the shifter
// if shift is zero (it is preserved)

// arithmetic its the carry out of the alu
// and V is somehow set?


// so for logical ones we need the shifter to return
// the carray out and use that to set C
// unless its zero then its preserved

// for arithmetic we need to set C in a sub function
// based on the carry

// and V  needs to be set but im not sure how




*/

void Cpu::arm_data_processing(uint32_t opcode)
{

    int cycles = 1; // 1 cycle for normal data processing

    int rd = (opcode >> 12) & 0xf;


    int rn = (opcode >> 16) & 0xf;

    uint32_t op1 = regs[rn];

    if(rn == PC)
    {
        // pc += 8 if used as operand
        op1 += 4;
    }

    // if s bit is set update flags
    bool update_flags = is_set(opcode,20);

    // default to preserve the carry
    // incase of a zero shift
    bool shift_carry = is_set(cpsr,C_BIT);


    uint32_t op2;

    // ror shifted immediate in increments of two
    if(is_set(opcode,25)) 
    {
        // how to calc the carry?
        const int imm = opcode & 0xff;
        const int shift = ((opcode >> 8) & 0xf)*2;

        if(shift != 0)
        {
            shift_carry = is_set(imm,shift-1);
        }
        // is this immediate?
        op2 = rotr(imm,shift);
    }

    else // shifted register 
    {
        auto type = static_cast<shift_type>((opcode >> 5 ) & 0x3);



        // immediate is allways register rm
        int rm = opcode & 0xf;
        uint32_t imm = regs[rm];

        if(rm == PC)
        {
            // pc + 8 if used as operand
            imm += 4; 
        }


        uint32_t shift_ammount = 0;
        // shift ammount is a register
        if(is_set(opcode,4))
        {
            // bottom byte of rs (no r15)
            int rs = (opcode >> 8) & 0xf;

            shift_ammount = regs[rs] & 0xff; 

            // if a reg shift ammount is used its +12
            if(rm == PC)
            {
                imm += 4; 
            }

            if(rn == PC)
            {
                op1 += 4;
            }
        }

        else // shift ammount is an 5 bit int
        {
            shift_ammount = (opcode >> 7) & 0x1f;
        }


        op2 = barrel_shift(type,imm,shift_ammount,shift_carry,!is_set(opcode,4));
    }



    if(rd == PC) // pc writen two extra cycle
    {
        // 1s + 1n if r15 loaded
        cycles += 2;
    }


    if(update_flags && rd == PC)
    {
        // ignored by real hardware but probably indicates a error
        if(arm_mode >= cpu_mode::user)
        {

        }
        
        else
        {
            set_cpsr(status_banked[static_cast<int>(arm_mode)]);
        }
    }
    
    // switch on the opcode to decide what to do
    int op = (opcode >> 21) & 0xf;
    switch(op)
    {
        case 0x0: //and
        {
            regs[rd] = logical_and(op1,op2,update_flags);
            if(update_flags)
            {
                cpsr = shift_carry? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT);
            }
            break;
        }

        case 0x1: // eor
        {
            regs[rd] = logical_eor(op1,op2,update_flags);
            if(update_flags)
            {
                cpsr = shift_carry? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT);
            }
            break;            
        }

        case 0x2: // sub
        {
            regs[rd] = sub(op1,op2,update_flags);
            break;
        }

        case 0x3: // rsb
        {
            regs[rd] = sub(op2,op1,update_flags);
            break;
        }

        case 0x4: // add
        {
            regs[rd] = add(op1,op2,update_flags);
            break;
        }


        case 0x5: // adc
        {
            regs[rd] = adc(op1,op2,update_flags);
            break;           
        }

        case 0x6: // sbc
        {
            regs[rd] = sbc(op1,op2,update_flags);
            break;
        }

        case 0x7: // rsc
        {
            regs[rd] = sbc(op2,op1,update_flags);
            break;
        }

        case 0x8: // tst (and without writeback)
        {
            logical_and(op1,op2,update_flags);
            if(update_flags)
            {
                cpsr = shift_carry? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT);
            }             
            break;
        }


        case 0x9: // teq
        {
            logical_eor(op1,op2,update_flags);
            if(update_flags)
            {
                cpsr = shift_carry? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT);
            }            
            break;
        }

        case 0xa: // cmp
        {
            sub(op1,op2,update_flags);
            break;
        }

        case 0xb: // cmn
        {
            add(op1,op2,update_flags);
            break;            
        }

        case 0xc: //orr
        {
            regs[rd] = logical_or(op1,op2,update_flags);
            if(update_flags)
            {
                cpsr = shift_carry? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT);
            }            
            break;
        }

        case 0xd: // mov
        {
            regs[rd] = op2;
            // V preserved
            if(update_flags) // if the set cond bit is on
            {
                set_nz_flag(regs[rd]);
                // carry is that of the shift oper
                cpsr = shift_carry? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT);                 
            }
            break;
        }


        case 0xe: // bic
        {
            regs[rd] = bic(op1,op2,update_flags);
            if(update_flags)
            {
                cpsr = shift_carry? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT);
            }            
            break;
        }

        case 0xf: // mvn
        {
            regs[rd] = ~op2;
            // V preserved
            if(update_flags) // if the set cond bit is on
            {
                set_nz_flag(regs[rd]);
                // carry is that of the shift oper
                cpsr = shift_carry? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT);                 
            }
            break;
        }

        default:
        {
            printf("cpu unknown data processing instruction %x!\n",op);
            arm_unknown(opcode);
            break;
        }
    }


    cycle_tick(cycles);
}


// bx 
void Cpu::arm_branch_and_exchange(uint32_t opcode)
{

    int rn = opcode & 0xf;

    // if bit 0 of rn is a 1
    // subsequent instrs decoded as thumb
    is_thumb = regs[rn] & 1;

    cpsr = is_thumb? set_bit(cpsr,5) : deset_bit(cpsr,5);

    // branch
    if(is_thumb)
    {
        regs[PC] = regs[rn] & ~1;
    }

    else
    {
        regs[PC] = regs[rn] & ~3;
    }

    // 2s + 1n
    cycle_tick(3);
}


// halfword doubleword signed data transfer
// <-- handle instr timings
void Cpu::arm_hds_data_transfer(uint32_t opcode)
{
    bool p = is_set(opcode,24);
    bool u = is_set(opcode,23);
    bool i = is_set(opcode,22);
    bool l = is_set(opcode,20);
    int rn = (opcode >> 16) & 0xf;
    int rd = (opcode >> 12) & 0xf;
    int op = (opcode >> 5) & 0x3;
    bool w = is_set(opcode,21);



    uint32_t addr = regs[rn];

    // pc + 8
    if(rn == PC) 
    { 
        addr += 4;
    }


    uint32_t offset;

    if(!i) // just this?
    {
        int rm = opcode & 0xf;
        offset = regs[rm];
    }

    else
    {
        uint8_t imm = opcode & 0xf;
        imm |= (opcode >> 4) & 0xf0;
        offset = imm;
    }



    if(p) // pre
    {
        addr += u? offset : -offset;
    }

    int cycles = 0;

    uint32_t value = regs[rd];
    if(rd == PC)
    {
        // pc + 12
        value += 8;
        cycles += 2; // extra 1s + 1n for pc
    }


    // now we choose between load and store
    // and just switch on the opcode
    if(l) // load
    {
        switch(op)
        {
            case 0:
            {
                printf("hds illegal load op: %08x\n",regs[PC]);
                break;
            }

            case 1: // ldrh
            {
                regs[rd] = mem.read_memt<uint16_t>(addr);
                cycle_tick(cycles+3); // 1s + 1n + 1i
                break;
            }

            case 2: // ldrsb
            {
                regs[rd] = sign_extend<uint32_t>(mem.read_memt<uint8_t>(addr),8);
                cycle_tick(cycles+3); // 1s + 1n + 1i
                break;
            }

            case 3: // ldrsh
            {
                regs[rd] = sign_extend<uint32_t>(mem.read_memt<uint16_t>(addr),16);
                cycle_tick(cycles+3); // 1s + 1n + 1i
                break;
            }
        }
    }

    else // store
    {
        switch(op)
        {
            case 1: // strh
            {
                mem.write_memt<uint16_t>(addr,value);
                cycle_tick(2); // 2n cycles
                break;
            }
            
            default: // doubleword ops not supported on armv4
            {
                auto err = fmt::format("hds illegal store op: {:08x}\n",regs[PC]);
                throw std::runtime_error(err);
            }

        }
    }


    // handle any write backs that occur
    // arm says we cant use the same rd and base with a writeback
    if(rn != rd) 
    {
        if(!p) // post allways do a writeback
        {
            regs[rn] += u? offset : -offset;
        }

        else if(w) // writeback
        {
            regs[rn] = addr;
        }
    }
}

// ldr , str
void Cpu::arm_single_data_transfer(uint32_t opcode)
{

    int cycles = 0;

    bool load = is_set(opcode,20);
    bool w = is_set(opcode,21); // write back
    bool p = is_set(opcode,24); // pre index

    auto cur_mode = arm_mode;
    bool mode_change = !p && w; // T
    if(mode_change)  // T force user mode
    {
        switch_mode(cpu_mode::user);
    }


    int rd = (opcode >> 12) & 0xf;
    int rn = (opcode >> 16) & 0xf;

    uint32_t addr = regs[rn];

    // due to prefetch pc is += 8; at this stage
    if(rn == PC) 
    { 
        addr += 4;
    }

    uint32_t offset;

    if(is_set(opcode,25))
    {
        auto type = static_cast<shift_type>((opcode >> 5 ) & 0x3);


        // immediate is allways register rm
        int rm = opcode & 0xf;
        uint32_t imm = regs[rm];


        // register specified shift ammounts are not allowed
        int shift_ammount = (opcode >> 7) & 0x1f;
        
        bool carry = is_set(cpsr,C_BIT);
        offset = barrel_shift(type,imm,shift_ammount,carry,true);
    }

    else // immeditate
    {
        // 12 bit immediate
        offset = opcode & 0xfff;
    }


    //byte / word bit
    bool is_byte = is_set(opcode,22);

    // up or down bit decides wether we add
    // or subtract the offest
    bool u = is_set(opcode,23);

    // up / down bit decides wether to add or subtract
    // the offset
    if(p)
    {
        addr += u? offset : -offset;
    }



    // perform the specifed memory access
    if(load) // ldr
    {
        if(is_byte)
        {
            regs[rd] = mem.read_memt<uint8_t>(addr);
        }

        else // ldr and swp use rotated reads
        {
            regs[rd] = mem.read_memt<uint32_t>(addr);
            regs[rd] = rotr(regs[rd],(addr&3)*8);
        }
        cycles = 3; // 1s + 1n + 1i

        if(rd == PC)
        {
            cycles += 2; // 1s + 1n if pc loaded
        }
    }

    else // str 
    {
        uint32_t v = regs[rd];

        // due to prefetch pc is now higher
        // (pc+12)
        if(rd == PC)
        {
            v += 8;
        }

		if(is_byte)
		{
			mem.write_memt<uint8_t>(addr,v);
		}
		
		
		else
		{
			mem.write_memt<uint32_t>(addr,v);
		}
        cycles = 2; // 2 N cycles for a store 

    }


    // handle any write backs that occur
    // arm says we cant use the same rd and base with a writeback
    if(rn != rd) 
    {
        if(!p) // post allways do a writeback
        {
            regs[rn] += u? offset : -offset;
        }

        else if(w) // writeback
        {
            regs[rn] = addr;
        }
    }

    // we forced it to operate in user so now we need to switch it back
    if(mode_change)
    {
        switch_mode(cur_mode);
    }


    cycle_tick(cycles);
}

}