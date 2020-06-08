#include <gba/cpu.h>
#include <gba/memory.h>
#include <gba/disass.h>

namespace gameboyadvance
{

uint16_t Cpu::fetch_thumb_opcode()
{
    // ignore the pipeline for now
    regs[PC] &= ~1;
    uint16_t opcode = mem.read_memt<uint16_t>(regs[PC]);
    regs[PC] += ARM_HALF_SIZE;
    return opcode;
}

void Cpu::exec_thumb()
{
    uint16_t op = fetch_thumb_opcode();

    execute_thumb_opcode(op);
}

void Cpu::execute_thumb_opcode(uint16_t instr)
{
    // get the bits that determine the kind of instr it is
    uint8_t op = (instr >> 8) & 0xff;

    // call the function from our opcode table
    std::invoke(thumb_opcode_table[op],this,instr);    
}



void Cpu::thumb_unknown(uint16_t opcode)
{
    uint8_t op = get_thumb_opcode_bits(opcode);
    auto err = fmt::format("[cpu-thumb {:08x}] unknown opcode {:04x}:{:x}\n",regs[PC],opcode,op);
    throw std::runtime_error(err);
}


void Cpu::thumb_load_store_sp(uint16_t opcode)
{
    uint32_t nn = (opcode & 0xff) * 4;
    int rd = (opcode >> 8) & 0x7;
    bool l = is_set(opcode,11);

    uint32_t addr = regs[SP] + nn;

    if(l)
    {
        regs[rd] = mem.read_memt<uint32_t>(addr);
        regs[rd] = rotr(regs[rd],(addr&3)*8);
        cycle_tick(3); // 1s + 1n + 1i for ldr        
    }

    else
    {
        mem.write_memt<uint32_t>(addr,regs[rd]);
        cycle_tick(2); // 2s for str
    }

}


void Cpu::thumb_sp_add(uint16_t opcode)
{
    bool u = !is_set(opcode,7);
    uint32_t nn = (opcode & 127) * 4;

    regs[SP] += u? nn : -nn;

    cycle_tick(1); // 1 s cycle    
}

// start here
// software interrupt (figure out interrupt vectors and what damb mode it swaps too)
// ^ not sure im reading from the right place for the vector table
void Cpu::thumb_swi(uint16_t opcode)
{
    // nn is ignored by hardware
    UNUSED(opcode);
    write_log(debug,"[cpu-thumb: {:08x}] swi {:x}",regs[PC],opcode & 0xff);

    int idx = static_cast<int>(cpu_mode::supervisor);

    // spsr for supervisor = cpsr
    status_banked[idx] = cpsr;

    // lr in supervisor mode set to return addr
    hi_banked[static_cast<int>(idx)][1] = regs[PC];

    // supervisor mode switch
    switch_mode(cpu_mode::supervisor);

    
    // switch to arm mode
    is_thumb = false; // switch to arm mode
    cpsr = deset_bit(cpsr,5); // toggle thumb in cpsr
    cpsr = set_bit(cpsr,7); //set the irq bit to mask interrupts

    // branch to interrupt vector
    regs[PC] = 0x8;
    cycle_tick(3); // 2s + 1n;
}

void Cpu::thumb_get_rel_addr(uint16_t opcode)
{
    uint32_t offset = (opcode & 0xff) * 4;
    int rd = (opcode >> 8) & 0x7;
    bool pc = !is_set(opcode,11);

    if(pc)
    {
        regs[rd] = ((regs[PC]+2) & ~2) + offset;
    }

    else
    {
        regs[rd] = regs[SP] + offset;
    }
    cycle_tick(1); // 1 s cycle
}

void Cpu::thumb_load_store_sbh(uint16_t opcode)
{
    int ro = (opcode >> 6) & 0x7;
    int rb = (opcode >> 3) & 0x7;
    int rd = opcode & 0x7;
    int op = (opcode >> 10) & 0x3;

    uint32_t addr = regs[rb] + regs[ro];

    switch(op)
    {
        case 0: // strh
        {
            mem.write_memt<uint16_t>(addr,regs[rd]);
            cycle_tick(2); // 2n for str
            break;
        }

        case 1: // ldsb
        {
            regs[rd] = sign_extend<uint32_t>(mem.read_memt<uint8_t>(addr),8);
            cycle_tick(3); // 1s + 1n + 1i
            break;
        }

        case 2: // ldrh
        {
            regs[rd] = mem.read_memt<uint16_t>(addr);
            // result rotated right by 8 on arm7 if unaligned 
            regs[rd] = rotr(regs[rd],8*(addr&1)); 
            
            cycle_tick(3); // 1s + 1n + 1i
            break;
        }

        case 3: //ldsh
        {
            if(!(addr & 1)) // is aligned
            {
                regs[rd] = sign_extend<uint32_t>(mem.read_memt<uint16_t>(addr),16);
            }

            else // unaligned
            {
                regs[rd] = sign_extend<uint32_t>(mem.read_memt<uint8_t>(addr),8);
            }

            cycle_tick(3); // 1s + 1n + 1i
            break;
        }        
    }
}

void Cpu::thumb_load_store_reg(uint16_t opcode)
{
    int op = (opcode >> 10) & 0x3;
    int ro = (opcode >> 6) & 0x7;
    int rb = (opcode >> 3) & 0x7;
    int rd = opcode & 0x7;

    uint32_t addr = regs[rb] + regs[ro];

    switch(op)
    {
        case 0: // str
        {
            mem.write_memt<uint32_t>(addr,regs[rd]);
            cycle_tick(2); // 2n for str
            break;
        }

        case 1: //strb
        {
            mem.write_memt<uint8_t>(addr,regs[rd]);
            cycle_tick(2); // 2n for str
            break;
        }

        case 2: // ldr
        {
            regs[rd] = mem.read_memt<uint32_t>(addr);
            regs[rd] = rotr(regs[rd],(addr&3)*8);
            cycle_tick(3); // 1s + 1n + 1i for ldr
            break;
        }

        case 3: // ldrb
        {
            regs[rd] = mem.read_memt<uint8_t>(addr);
            cycle_tick(3); // 1s + 1n + 1i for ldr
            break;
        }
    }
}


void Cpu::thumb_branch(uint16_t opcode)
{
    auto offset = sign_extend<int32_t>(opcode & 0x7ff,11) * 2;
    regs[PC] += offset+ARM_HALF_SIZE;

    cycle_tick(3); // 2s +1n 
}

void Cpu::thumb_load_store_half(uint16_t opcode)
{
    int nn = ((opcode >> 6) & 0x1f) * 2;
    int rb = (opcode >> 3) & 0x7;
    int rd = opcode & 0x7;

    bool load = is_set(opcode,11);

    if(load) // ldrh
    {
        uint32_t addr = regs[rb] + nn;
        regs[rd] = mem.read_memt<uint16_t>(addr);
        // arm7 rotate by 8 if unaligned
        regs[rd] = rotr(regs[rd],8*(addr&1)); 
        cycle_tick(3); //1s +1n + 1i
    }   

    else //strh
    {
        mem.write_memt<uint16_t>(regs[rb]+nn,regs[rd]);
        cycle_tick(2); // 2n for str
    } 
}

void Cpu::thumb_push_pop(uint16_t opcode)
{
    const bool pop = is_set(opcode,11);

    const bool lr = is_set(opcode,8);

    const uint8_t reg_range = opcode & 0xff;

    int n = 0;

    if(pop)
    {
        for(int i = 0; i < 8; i++)
        {
            if(is_set(reg_range,i))
            {
                n++;
                regs[i] = mem.read_memt<uint32_t>(regs[SP]);
                regs[SP] += ARM_WORD_SIZE;
            }
        }

        // nS +1N +1I (pop) | (n+1)S +2N +1I(pop pc)
        if(lr)
        {
            regs[PC] = mem.read_memt<uint32_t>(regs[SP]) & ~1;
            regs[SP] += ARM_WORD_SIZE;
            cycle_tick((n+1) + 3);
        }

        else
        {
            cycle_tick(n + 2);
        }

    }


    else // push
    {
        // addresses allways handled increasing
        // in this case the cpu calcs how far back
        // it needs to go first
        for(int i = 0; i < 8; i++)
        {
            if(is_set(reg_range,i))
            {
                n++;
                regs[SP] -= ARM_WORD_SIZE;
            }
        }


        if(lr) 
        {
            n++;
            regs[SP] -= ARM_WORD_SIZE;
        }

        uint32_t addr = regs[SP];
        for(int i = 0; i < 8; i++)
        {
            if(is_set(reg_range,i))
            {
                mem.write_memt<uint32_t>(addr,regs[i]);
                addr += ARM_WORD_SIZE;
            }
        }

        if(lr)
        {
            mem.write_memt<uint32_t>(addr,regs[LR]);
        }

        // (n-1)S+2N (PUSH)
        cycle_tick((n-1) + 2);
    }

}

void Cpu::thumb_hi_reg_ops(uint16_t opcode)
{
    int rd = opcode & 0x7;
    int rs = (opcode >> 3) & 0x7;
    int op = (opcode >> 8) & 0x3;

    

    // 1s cycle min
    int cycles = 1;

    // can be used as bl/blx flag (not revlant for gba?)
    bool msbd = is_set(opcode,7);

    // bit 7 and 6 act as top bits of the reg
    rd = msbd? set_bit(rd,3) : rd;
    rs = is_set(opcode,6)? set_bit(rs,3) : rs;

    uint32_t rs_val = regs[rs];

    // if using PC its +4 ahead due to the pipeline
    if(rs == PC)
    {
        rs_val += 2;
    }


    uint32_t rd_val = regs[rd];

    //2s + 1n total if using pc as rd
    if(rd == PC)
    {
        cycles += 2;
        // if pc used at plus 4
        rd_val += 2;
    }


    // only cmp sets flags here!
    switch(op)
    {
        case 0b00: // add
        {
            regs[rd] = add(rd_val,rs_val,false);
            break;  
        }

        case 0b01: // cmp
        {
            // do sub and discard result
            sub(rd_val,rs_val,true);
            break;
        }

        case 0b10: // mov
        {
            regs[rd] = rs_val;
            break;
        }

        case 0b11: // bx
        {
            // if bit 0 of rn is a 1
            // subsequent instrs decoded as thumb
            is_thumb = rs_val & 1;

            // set the thumb bit
            cpsr = is_thumb? set_bit(cpsr,5) : deset_bit(cpsr,5);

            // branch
            regs[PC] = is_thumb? rs_val & ~1 : rs_val & ~3;
            cycles = 3; // 2s +1n for bx            
            break;
        }
    }

    cycle_tick(cycles);
}

void Cpu::thumb_alu(uint16_t opcode)
{
    int op = (opcode >> 6) & 0xf;
    int rs = (opcode >> 3) & 0x7;
    int rd = opcode & 0x7;



    switch(op)
    {

        case 0x0: // and
        {
            regs[rd] = logical_and(regs[rd],regs[rs],true);
            cycle_tick(1); // 1 s cycle
            break;
        }

        case 0x1: // eor
        {
            regs[rd] ^= regs[rs];
            set_nz_flag(regs[rd]);
            cycle_tick(1); // 1 s cycle
            break;
        }

        case 0x2: // lsl
        {
            bool c = is_set(cpsr,C_BIT);
            regs[rd] = lsl(regs[rd],regs[rs]&0xff,c);
            set_nz_flag(regs[rd]);
            cpsr = c? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT);
            cycle_tick(2); // 1s + 1i
            break;
        }

        case 0x3: // lsr 
        {
            bool c = is_set(cpsr,C_BIT);
            regs[rd] = lsr(regs[rd],regs[rs]&0xff,c,false);
            set_nz_flag(regs[rd]);
            cpsr = c? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT);
            cycle_tick(2); // 1s + 1i
            break;            
        }

        case 0x4: // asr
        {
            bool c = is_set(cpsr,C_BIT);
            regs[rd] = asr(regs[rd],regs[rs]&0xff,c,false);
            set_nz_flag(regs[rd]);
            cpsr = c? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT);   
            cycle_tick(2); // 1s + 1i
            break;         
        }

        case 0x5: // adc
        {
            regs[rd] = adc(regs[rd],regs[rs],true);
            cycle_tick(1); // 1 s cycle
            break;
        }

        case 0x6: // sbc
        {
            regs[rd] = sbc(regs[rd],regs[rs],true);
            cycle_tick(1); // 1 s cycle
            break;
        }

        
        case 0x7: // ror
        {
            bool c = is_set(cpsr,C_BIT);
            regs[rd] = ror(regs[rd],regs[rs]&0xff,c,false);
            set_nz_flag(regs[rd]);
            cpsr = c? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT);
            cycle_tick(2); // 1s + 1i
            break;
        }

        case 0x8: // tst
        {
            logical_and(regs[rd],regs[rs],true);
            cycle_tick(1); // 1 s cycle
            break;
        }

        case 0x9: // neg
        {
            regs[rd] = sub(0,regs[rs],true);
            cycle_tick(1); // 1 s cycle
            break;
        }

        case 0xa: // cmp
        {
            sub(regs[rd],regs[rs],true);
            cycle_tick(1); // 1 s cycle
            break;
        }

        case 0xb: // cmn
        {
            add(regs[rd],regs[rs],true);
            cycle_tick(1); // 1 s cycle
            break;
        }

        case 0xc: // orr
        {
            regs[rd] = logical_or(regs[rd],regs[rs],true);
            cycle_tick(1); // 1 s cycle
            break;
        }

        case 0xd: // mul
        {
            regs[rd] *= regs[rs];
            set_nz_flag(regs[rd]);
            cpsr = deset_bit(cpsr,C_BIT);
            cycle_tick(1); // needs timing fix
            break;
        }


        case 0xe: // bic
        {
            regs[rd] &= ~regs[rs];
            set_nz_flag(regs[rd]);
            cycle_tick(1); // 1 s cycle for bic
            break;
        }

        case 0xf: // mvn
        {
            regs[rd] = ~regs[rs];
            set_nz_flag(regs[rd]);                  
            cycle_tick(1); // 1 s cycle
            break;
        }
    }
}


void Cpu::thumb_multiple_load_store(uint16_t opcode)
{
    int rb = (opcode >> 8) & 0x7;
    bool load = is_set(opcode,11);

    uint8_t reg_range = opcode & 0xff;

    int n = 0;


    // empty r list store pc sb += 0x40
    if(reg_range == 0)
    {
        n++;


        // ldmia
        if(load)
        {
            regs[PC] = mem.read_memt<uint32_t>(regs[rb]);
        }

        //stmia
        else
        {
            mem.write_memt<uint32_t>(regs[rb],regs[PC]);
        }

        regs[rb] += 0x40;        
    }   

    else
    {
        for(int i = 0; i < 8; i++)
        {
            if(is_set(reg_range,i))
            {
                n++;
                // ldmia
                if(load)
                {
                    regs[i] = mem.read_memt<uint32_t>(regs[rb]);
                }
                //stmia
                else
                {
                    mem.write_memt<uint32_t>(regs[rb],regs[i]);
                }
                regs[rb] += ARM_WORD_SIZE;
            }
        }
    }

    
    if(!load)
    {
        // 2n plus (n-1)s
        cycle_tick(2 + (n-1));
    }

    else
    {
        // 1N + 1i + ns
        cycle_tick(2+n);
    }

}


void Cpu::thumb_ldst_imm(uint16_t opcode)
{
    int op = (opcode >> 11) & 3;

    int imm = (opcode >> 6) & 0x1f;

    int rb = (opcode >> 3) & 0x7;
    int rd = opcode & 0x7;

    // 1s + 1n + 1i for ldr
    // 2n for str
    switch(op)
    {
        case 0b00: // str
        {  
            mem.write_memt<uint32_t>((regs[rb]+imm*4),regs[rd]);
            cycle_tick(2);
            break;
        }

        case 0b01: // ldr
        {
            uint32_t addr = regs[rb]+imm*4;
            regs[rd] = mem.read_memt<uint32_t>(addr);
            regs[rd] = rotr(regs[rd],(addr&3)*8);
            cycle_tick(3);
            break;            
        }

        case 0b10: // strb
        {
            mem.write_memt<uint8_t>((regs[rb]+imm),regs[rd]);
            cycle_tick(2);
            break;
        }

        case 0b11: // ldrb
        {
            regs[rd] = mem.read_memt<uint8_t>((regs[rb]+imm));
            cycle_tick(3);       
            break;
        }
    }


}

void Cpu::thumb_add_sub(uint16_t opcode)
{
    int rd = opcode & 0x7;
    int rs = (opcode >> 3) & 0x7;
    int rn = (opcode >> 6) & 0x7; // can also be 3 bit imm
    int op = (opcode >> 9) & 0x3;

    switch(op)
    {
        case 0b00: // add reg
        { 
            regs[rd] = add(regs[rs],regs[rn],true);
            break;
        }
        case 0b01: // sub reg
        { 
            regs[rd] = sub(regs[rs],regs[rn],true);
            break;
        }        
        case 0b10: // add imm
        { 
            regs[rd] = add(regs[rs],rn,true);
            break;
        }        
        case 0b11: // sub imm
        { 
            regs[rd] = sub(regs[rs],rn,true);
            break;
        }        
    }

    // 1 s cycle
    cycle_tick(1);
}

void Cpu::thumb_long_bl(uint16_t opcode)
{
    bool first = !is_set(opcode,11);

    int32_t offset = opcode & 0x7ff; // offset is 11 bits

    if(first)
    {
        // sign extend offset shifted by 12
        // add to pc plus 4 store in lr
        offset <<= 12;
        offset = sign_extend<int32_t>(offset,23);
        regs[LR] = (regs[PC]+2) + offset;
        cycle_tick(1); // 1S
    }

    else // 2nd instr
    {
        // tmp = next instr addr
        uint32_t tmp = regs[PC];
        // pc = lr + offsetlow << 1
        regs[PC] = (regs[LR] + (offset << 1)) & ~1;
        // lr = tmp | 1
        regs[LR] = tmp | 1;
        cycle_tick(3); //2S+1N cycle
        write_log(debug,"[cpu-thumb {:08x}] call {:08x}",tmp,regs[PC]);
    }

}

void Cpu::thumb_mov_reg_shift(uint16_t opcode)
{
    int rd = opcode & 0x7;
    int rs = (opcode >> 3) & 0x7;
    int n = (opcode >> 6) & 0x1f;

    auto type = static_cast<shift_type>((opcode >> 11) & 0x3);

    bool did_carry = is_set(cpsr,C_BIT);

    regs[rd] = barrel_shift(type,regs[rs],n,did_carry,true);

    set_nz_flag(regs[rd]);


    cpsr = did_carry? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT); 


    // 1 s cycle
    cycle_tick(1);
}

void Cpu::thumb_mcas_imm(uint16_t opcode)
{
    int op = (opcode >> 11) & 0x3;
    int rd = (opcode >> 8) & 0x7;
    uint8_t imm = opcode & 0xff;

    switch(op)
    {
        case 0b00: // mov
        {
            regs[rd] = imm;
            set_nz_flag(regs[rd]);            
            break;
        }

        case 0b01: //cmp
        {
            sub(regs[rd],imm,true);
            break;
        }

        case 0b10: // add
        {
            regs[rd] = add(regs[rd],imm,true);
            break;
        }

        case 0b11: // sub
        {
            regs[rd] = sub(regs[rd],imm,true);
            break;
        }
    }

    // 1 s cycle
    cycle_tick(1);
}


void Cpu::thumb_cond_branch(uint16_t opcode)
{
    int8_t offset = opcode & 0xff;
    uint32_t addr = (regs[PC]+2) + offset*2;
    int cond = (opcode >> 8) & 0xf;

    // if branch taken 2s +1n cycles
    if(cond_met(cond))
    {
        regs[PC] = addr & ~1;
        cycle_tick(3);
    }

    // else 1s
    else 
    {
        cycle_tick(1);
    }
}

void Cpu::thumb_ldr_pc(uint16_t opcode)
{
    int rd = (opcode >> 8) & 0x7;

    // 0 - 1020 in offsets of 4
    uint32_t offset = (opcode & 0xff) * 4;

    // pc will have bit two deset to ensure word alignment
    // pc is + 4 ahead of current instr
    uint32_t addr = ((regs[PC] + 2) & ~2) + offset;

    regs[rd] = mem.read_memt<uint32_t>(addr);


    // takes 2s + 1n cycles
    cycle_tick(3);
}

}