#include <gba/gba.h>


// i need to properly handle pipeline effects
// within thumb mode as well as in arm mode

namespace gameboyadvance
{

void Cpu::init_thumb_opcode_table()
{
    thumb_opcode_table.resize(256);

    for(int i = 0; i < 256; i++)
    {
        thumb_opcode_table[i] = &Cpu::thumb_unknown;

        // THUMB.1: move shifted register
        // top 3 bits unset
        if(((i >> 5) & 0b111) == 0b000 && ((i >> 3) & 0b11) != 0b11)
        {
            thumb_opcode_table[i] = &Cpu::thumb_mov_reg_shift;
        }

        // THUMB.2: add/subtract
        else if(((i >> 3) & 0b11111) == 0b00011)
        {
            thumb_opcode_table[i] = &Cpu::thumb_add_sub;
        }



        // THUMB.3: move/compare/add/subtract immediate
        else if(((i >> 5) & 0b111) == 0b001)
        {
            thumb_opcode_table[i] = &Cpu::thumb_mcas_imm;
        }


        // THUMB.4: ALU operations
        else if(((i >> 2) & 0b111111) == 0b010000)
        {
            thumb_opcode_table[i] = &Cpu::thumb_alu;
        }

        // THUMB.5: Hi register operations/branch exchange
        else if(((i >> 2) & 0b111111) == 0b010001)
        {
           thumb_opcode_table[i] = &Cpu::thumb_hi_reg_ops;
        }

        // THUMB.6: load PC-relative
        else if(((i >> 3) & 0b11111) ==  0b01001)
        {
            thumb_opcode_table[i] = &Cpu::thumb_ldr_pc;
        }


        // THUMB.7: load/store with register offset
        else if(((i >> 4) & 0b1111) == 0b0101 && !is_set(i,1))
        {
           thumb_opcode_table[i] = &Cpu::thumb_load_store_reg;
        }

        // THUMB.8: load/store sign-extended byte/halfword
        else if(((i >> 4) & 0b1111) == 0b0101 && is_set(i,1))
        {
            thumb_opcode_table[i] = &Cpu::thumb_load_store_sbh;
        }

        // THUMB.9: load/store with immediate offset
        else if(((i>>5) & 0b111) == 0b011)
        {
            thumb_opcode_table[i] = &Cpu::thumb_ldst_imm;
        }



        //THUMB.10: load/store halfword
        else if(((i >> 4) & 0b1111) == 0b1000)
        {
            thumb_opcode_table[i] = &Cpu::thumb_load_store_half;
        }

        // THUMB.11: load/store SP-relative
        else if(((i >> 4) & 0b1111) == 0b1001)
        {
            thumb_opcode_table[i] = &Cpu::thumb_load_store_sp;
        }

        // THUMB.12: get relative address
        else if(((i >> 4) & 0b1111) == 0b1010)
        {
            thumb_opcode_table[i] = &Cpu::thumb_get_rel_addr;
        }
        


        // THUMB.13: add offset to stack pointer
        else if(i == 0b10110000)
        {
            thumb_opcode_table[i] = &Cpu::thumb_sp_add;
        }



        //THUMB.14: push/pop registers
        else if(((i >> 4) & 0b1111) == 0b1011 
            && ((i >> 1) & 0b11) == 0b10)
        {
            thumb_opcode_table[i] = &Cpu::thumb_push_pop;
        }

        //  THUMB.15: multiple load/store
        else if(((i >> 4) & 0b1111) == 0b1100)
        {
            thumb_opcode_table[i] = &Cpu::thumb_multiple_load_store;
        }

        // THUMB.16: conditional branch
        else if(((i >> 4)  & 0b1111) == 0b1101 && (i & 0xf) != 0xf)
        {
            thumb_opcode_table[i] = &Cpu::thumb_cond_branch;
        }

        // THUMB.17: software interrupt and breakpoint
        else if(i == 0b11011111)
        {
            thumb_opcode_table[i] = &Cpu::thumb_swi;
        }


        // THUMB.18: unconditional branch
        else if(((i >> 3) & 0b11111) == 0b11100)
        {
            thumb_opcode_table[i] = &Cpu::thumb_branch;
        }
 
        // THUMB.19: long branch with link
        else if(((i >> 4) & 0b1111) == 0b1111)
        {
            thumb_opcode_table[i] = &Cpu::thumb_long_bl;
        }

        else 
        {
            thumb_opcode_table[i] = &Cpu::thumb_unknown;
        }                 
    }
}


// todo remove preftech hacks
void Cpu::thumb_fill_pipeline() // need to verify this...
{
    pipeline[0] = mem.read_memt<uint16_t>(regs[PC]);
    regs[PC] += ARM_HALF_SIZE;
    pipeline[1] = mem.read_memt<uint16_t>(regs[PC]);
}


uint16_t Cpu::fetch_thumb_opcode()
{
    const uint16_t opcode = pipeline[0];
    pipeline[0] = pipeline[1];
    regs[PC] += ARM_HALF_SIZE; 
    pc_actual += ARM_HALF_SIZE; 
    pipeline[1] = mem.read_memt<uint16_t>(regs[PC]);
    return opcode;
}

void Cpu::write_pc_thumb(uint32_t v)
{
    regs[PC] = v & ~1;
    thumb_fill_pipeline(); // fill the intitial cpu pipeline
}

void Cpu::exec_thumb()
{
    const auto op = fetch_thumb_opcode();

    execute_thumb_opcode(op);
}

void Cpu::execute_thumb_opcode(uint16_t instr)
{
    // get the bits that determine the kind of instr it is
    const uint8_t op = (instr >> 8) & 0xff;

    // call the function from our opcode table
    std::invoke(thumb_opcode_table[op],this,instr);  
}



void Cpu::thumb_unknown(uint16_t opcode)
{
    uint8_t op = get_thumb_opcode_bits(opcode);
    auto err = fmt::format("[cpu-thumb {:08x}] unknown opcode {:04x}:{:x}\n{}\n",regs[PC],opcode,op,disass.disass_thumb(get_pc()));
    throw std::runtime_error(err);
}


void Cpu::thumb_load_store_sp(uint16_t opcode)
{
    const uint32_t nn = (opcode & 0xff) * 4;
    const auto rd = (opcode >> 8) & 0x7;
    const bool l = is_set(opcode,11);

    const auto addr = regs[SP] + nn;

    if(l)
    {
        regs[rd] = mem.read_memt<uint32_t>(addr);
        regs[rd] = rotr(regs[rd],(addr&3)*8);
        internal_cycle(); // internal for writeback       
    }

    else
    {
        mem.write_memt<uint32_t>(addr,regs[rd]);
    } 
}


void Cpu::thumb_sp_add(uint16_t opcode)
{
    const bool u = !is_set(opcode,7);
    const uint32_t nn = (opcode & 127) * 4;

    regs[SP] += u? nn : -nn; 
}

void Cpu::thumb_swi(uint16_t opcode)
{
    //printf("swi %08x: %08x\n",get_pc(),opcode);

    // nn is ignored by hardware
    UNUSED(opcode);
    write_log(debug,"[cpu-thumb: {:08x}] swi {:x}",regs[PC],opcode & 0xff);

    const auto idx = static_cast<int>(cpu_mode::supervisor);

    // spsr for supervisor = cpsr
    status_banked[idx] = get_cpsr();

    // lr in supervisor mode set to return addr
    hi_banked[static_cast<int>(idx)][1] = pc_actual;

    // supervisor mode switch
    switch_mode(cpu_mode::supervisor);

    // switch to arm mode
    is_thumb = false; // switch to arm mode
    cpsr = deset_bit(cpsr,5); // toggle thumb in cpsr
    cpsr = set_bit(cpsr,7); //set the irq bit to mask interrupts

    // branch to interrupt vector
    write_pc(0x8);
}

void Cpu::thumb_get_rel_addr(uint16_t opcode)
{
    const uint32_t offset = (opcode & 0xff) * 4;
    const auto rd = (opcode >> 8) & 0x7;
    const bool pc = !is_set(opcode,11);

    if(pc)
    {
        regs[rd] = (regs[PC] & ~2) + offset;
    }

    else
    {
        regs[rd] = regs[SP] + offset;
    }
}

void Cpu::thumb_load_store_sbh(uint16_t opcode)
{
    const auto ro = (opcode >> 6) & 0x7;
    const auto rb = (opcode >> 3) & 0x7;
    const auto rd = opcode & 0x7;
    const auto op = (opcode >> 10) & 0x3;

    const auto addr = regs[rb] + regs[ro];

    switch(op)
    {
        case 0: // strh
        {
            mem.write_memt<uint16_t>(addr,regs[rd]);
            break;
        }

        case 1: // ldsb
        {
            regs[rd] = sign_extend<uint32_t>(mem.read_memt<uint8_t>(addr),8);
            internal_cycle(); // internal cycle for reg writeback
            break;
        }

        case 2: // ldrh
        {
            regs[rd] = mem.read_memt<uint16_t>(addr);
            // result rotated right by 8 on arm7 if unaligned 
            regs[rd] = rotr(regs[rd],8*(addr&1)); 
            internal_cycle(); // internal cycle for reg writeback
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
            internal_cycle(); // internal cycle for reg writeback
            break;
        }        
    }
}

void Cpu::thumb_load_store_reg(uint16_t opcode)
{
    const auto op = (opcode >> 10) & 0x3;
    const auto ro = (opcode >> 6) & 0x7;
    const auto rb = (opcode >> 3) & 0x7;
    const auto rd = opcode & 0x7;

    const auto addr = regs[rb] + regs[ro];

    switch(op)
    {
        case 0: // str
        {
            mem.write_memt<uint32_t>(addr,regs[rd]);
            break;
        }

        case 1: //strb
        {
            mem.write_memt<uint8_t>(addr,regs[rd]);
            break;
        }

        case 2: // ldr
        {
            regs[rd] = mem.read_memt<uint32_t>(addr);
            regs[rd] = rotr(regs[rd],(addr&3)*8);
            internal_cycle(); // for reg writeback
            break;
        }

        case 3: // ldrb
        {
            regs[rd] = mem.read_memt<uint8_t>(addr);
            internal_cycle(); // for reg writeback
            break;
        }
    }
}


void Cpu::thumb_branch(uint16_t opcode)
{
    const auto offset = sign_extend<int32_t>(opcode & 0x7ff,11) * 2;
    
    const auto old = pc_actual-2;

    write_pc(regs[PC] + offset);

    if(old == pc_actual)
    {
        scheduler.skip_to_event();               
    }
}

void Cpu::thumb_load_store_half(uint16_t opcode)
{
    const auto nn = ((opcode >> 6) & 0x1f) * 2;
    const auto rb = (opcode >> 3) & 0x7;
    const auto rd = opcode & 0x7;
    const bool load = is_set(opcode,11);

    if(load) // ldrh
    {
        const auto addr = regs[rb] + nn;
        regs[rd] = mem.read_memt<uint16_t>(addr);
        // arm7 rotate by 8 if unaligned
        regs[rd] = rotr(regs[rd],8*(addr&1)); 
        internal_cycle(); // internal cycle for writeback
    }   

    else //strh
    {
        mem.write_memt<uint16_t>(regs[rb]+nn,regs[rd]);
    } 
}

void Cpu::thumb_push_pop(uint16_t opcode)
{
    const bool pop = is_set(opcode,11);
    const bool lr = is_set(opcode,8);
    const uint8_t reg_range = opcode & 0xff;


    // todo (emtpy r list timings here)
    if(pop)
    {
        for(int i = 0; i < 8; i++)
        {
            if(is_set(reg_range,i))
            {
                regs[i] = mem.read_memt<uint32_t>(regs[SP]);
                regs[SP] += ARM_WORD_SIZE;
            }
        }

        // final internal cycle for load
        internal_cycle();

        // nS +1N +1I (pop) | (n+1)S +2N +1I(pop pc)
        if(lr)
        {
            write_pc(mem.read_memt<uint32_t>(regs[SP]));
            regs[SP] += ARM_WORD_SIZE;
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
                regs[SP] -= ARM_WORD_SIZE;
            }
        }


        if(lr) 
        {
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
    }

}

void Cpu::thumb_hi_reg_ops(uint16_t opcode)
{
    auto rd = opcode & 0x7;
    auto rs = (opcode >> 3) & 0x7;
    const auto op = (opcode >> 8) & 0x3;

    // can be used as bl/blx flag (not revlant for gba?)
    const bool msbd = is_set(opcode,7);

    // bit 7 and 6 act as top bits of the reg
    rd = msbd? set_bit(rd,3) : rd;
    rs = is_set(opcode,6)? set_bit(rs,3) : rs;

    const auto rs_val = regs[rs];
    const auto rd_val = regs[rd];

    // only cmp sets flags here!
    switch(op)
    {
        case 0b00: // add
        {
            regs[rd] = add(rd_val,rs_val,false);

            if(rd == PC)
            {
                write_pc(regs[PC]);
            }
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
            if(rd == PC)
            {
                write_pc(regs[PC]);
            }
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
            write_pc(rs_val);
            break;
        }
    }
}

void Cpu::thumb_alu(uint16_t opcode)
{
    const auto op = (opcode >> 6) & 0xf;
    const auto rs = (opcode >> 3) & 0x7;
    const auto rd = opcode & 0x7;

    switch(op)
    {

        case 0x0: // and
        {
            regs[rd] = logical_and(regs[rd],regs[rs],true);
            break;
        }

        case 0x1: // eor
        {
            regs[rd] ^= regs[rs];
            set_nz_flag(regs[rd]);
            break;
        }

        case 0x2: // lsl
        {
            regs[rd] = lsl(regs[rd],regs[rs]&0xff,flag_c);
            set_nz_flag(regs[rd]);
            internal_cycle(); // reg shift
            break;
        }

        case 0x3: // lsr 
        {
            regs[rd] = lsr(regs[rd],regs[rs]&0xff,flag_c,false);
            set_nz_flag(regs[rd]);
            internal_cycle(); // reg shift
            break;            
        }

        case 0x4: // asr
        {
            regs[rd] = asr(regs[rd],regs[rs]&0xff,flag_c,false);
            set_nz_flag(regs[rd]); 
            internal_cycle(); // reg shift
            break;         
        }

        case 0x5: // adc
        {
            regs[rd] = adc(regs[rd],regs[rs],true);
            break;
        }

        case 0x6: // sbc
        {
            regs[rd] = sbc(regs[rd],regs[rs],true);
            break;
        }

        
        case 0x7: // ror
        {
            regs[rd] = ror(regs[rd],regs[rs]&0xff,flag_c,false);
            set_nz_flag(regs[rd]);
            internal_cycle(); // reg shift
            break;
        }

        case 0x8: // tst
        {
            logical_and(regs[rd],regs[rs],true);
            break;
        }

        case 0x9: // neg
        {
            regs[rd] = sub(0,regs[rs],true);
            break;
        }

        case 0xa: // cmp
        {
            sub(regs[rd],regs[rs],true);
            break;
        }

        case 0xb: // cmn
        {
            add(regs[rd],regs[rs],true);
            break;
        }

        case 0xc: // orr
        {
            regs[rd] = logical_or(regs[rd],regs[rs],true);
            break;
        }

        case 0xd: // mul
        {
            regs[rd] *= regs[rs];
            set_nz_flag(regs[rd]);
            flag_c = false;
            do_mul_cycles(regs[rs]);
            break;
        }


        case 0xe: // bic
        {
            regs[rd] &= ~regs[rs];
            set_nz_flag(regs[rd]);
            break;
        }

        case 0xf: // mvn
        {
            regs[rd] = ~regs[rs];
            set_nz_flag(regs[rd]);
            break;
        }
    }
}


void Cpu::thumb_multiple_load_store(uint16_t opcode)
{
    const auto rb = (opcode >> 8) & 0x7;
    const bool load = is_set(opcode,11);
    const auto reg_range = opcode & 0xff;


    // empty r list store pc, sb += 0x40
    if(reg_range == 0)
    {
        // ldmia
        if(load)
        {
            const auto v = mem.read_memt<uint32_t>(regs[rb]);
            internal_cycle();
            write_pc(v);
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

    
    // one final internal cycle for loads
    if(load)
    {
        internal_cycle();
    }
}


void Cpu::thumb_ldst_imm(uint16_t opcode)
{
    const auto op = (opcode >> 11) & 3;
    const auto imm = (opcode >> 6) & 0x1f;
    const auto rb = (opcode >> 3) & 0x7;
    const auto rd = opcode & 0x7;

    // 1s + 1n + 1i for ldr
    // 2n for str
    switch(op)
    {
        case 0b00: // str
        {  
            mem.write_memt<uint32_t>((regs[rb]+imm*4),regs[rd]);
            break;
        }

        case 0b01: // ldr
        {
            uint32_t addr = regs[rb]+imm*4;
            regs[rd] = mem.read_memt<uint32_t>(addr);
            regs[rd] = rotr(regs[rd],(addr&3)*8);
            internal_cycle(); // cycle for register writeback
            break;            
        }

        case 0b10: // strb
        {
            mem.write_memt<uint8_t>((regs[rb]+imm),regs[rd]);
            break;
        }

        case 0b11: // ldrb
        {
            regs[rd] = mem.read_memt<uint8_t>((regs[rb]+imm));
            internal_cycle(); // cycle for register writeback     
            break;
        }
    }
}

void Cpu::thumb_add_sub(uint16_t opcode)
{    
    const auto rd = opcode & 0x7;
    const auto rs = (opcode >> 3) & 0x7;
    const auto rn = (opcode >> 6) & 0x7; // can also be 3 bit imm
    const auto op = (opcode >> 9) & 0x3;

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
}

void Cpu::thumb_long_bl(uint16_t opcode)
{
    const bool first = !is_set(opcode,11);

    int32_t offset = opcode & 0x7ff; // offset is 11 bits

    if(first)
    {
        // sign extend offset shifted by 12
        // add to pc plus 4 store in lr
        offset <<= 12;
        offset = sign_extend<int32_t>(offset,23);
        regs[LR] = regs[PC] + offset;
    }

    else // 2nd instr
    {
        // tmp = next instr addr
        const uint32_t tmp = pc_actual;
        // pc = lr + offsetlow << 1
        write_pc((regs[LR] + (offset << 1)));
        // lr = tmp | 1
        regs[LR] = tmp | 1;
        write_log(debug,"[cpu-thumb {:08x}] call {:08x}",tmp,pc_actual);
        //printf("[%08x] call %08x\n",tmp,pc_actual);
    }
}

void Cpu::thumb_mov_reg_shift(uint16_t opcode)
{
    const auto rd = opcode & 0x7;
    const auto rs = (opcode >> 3) & 0x7;
    const auto n = (opcode >> 6) & 0x1f;

    const auto type = static_cast<shift_type>((opcode >> 11) & 0x3);

    regs[rd] = barrel_shift(type,regs[rs],n,flag_c,true);

    set_nz_flag(regs[rd]);
}

void Cpu::thumb_mcas_imm(uint16_t opcode)
{
    const auto op = (opcode >> 11) & 0x3;
    const auto rd = (opcode >> 8) & 0x7;
    const uint8_t imm = opcode & 0xff;

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
}


void Cpu::thumb_cond_branch(uint16_t opcode)
{
    // 1st cycle is branch calc overlayed with pipeline
    const int8_t offset = opcode & 0xff;
    const uint32_t addr = regs[PC] + offset*2;
    const auto cond = (opcode >> 8) & 0xf;

    if(cond_met(cond))
    {
        write_pc(addr);  
    }
}

void Cpu::thumb_ldr_pc(uint16_t opcode)
{
    const auto rd = (opcode >> 8) & 0x7;

    // 0 - 1020 in offsets of 4
    const uint32_t offset = (opcode & 0xff) * 4;

    // pc will have bit two deset to ensure word alignment
    // pc is + 4 ahead of current instr
    const uint32_t addr = (regs[PC] & ~2) + offset;

    regs[rd] = mem.read_memt<uint32_t>(addr);

    // internal cycle for load writeback
    internal_cycle();
}

}
