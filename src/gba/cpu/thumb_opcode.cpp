#include <gba/gba.h>
#include <gba/thumb_lut.h>


// i need to properly handle pipeline effects
// within thumb mode as well as in arm mode

namespace gameboyadvance
{


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

template<const int RD, const bool L>
void Cpu::thumb_load_store_sp(uint16_t opcode)
{
    const uint32_t nn = (opcode & 0xff) * 4;
    const auto addr = regs[SP] + nn;

    if constexpr(L)
    {
        regs[RD] = mem.read_memt<uint32_t>(addr);
        regs[RD] = rotr(regs[RD],(addr&3)*8);
        internal_cycle(); // internal for writeback       
    }

    else
    {
        mem.write_memt<uint32_t>(addr,regs[RD]);
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

template<const int RD, const bool IS_PC>
void Cpu::thumb_get_rel_addr(uint16_t opcode)
{
    const uint32_t offset = (opcode & 0xff) * 4;

    if constexpr(IS_PC)
    {
        regs[RD] = (regs[PC] & ~2) + offset;
    }

    else
    {
        regs[RD] = regs[SP] + offset;
    }
}

template<const int OP>
void Cpu::thumb_load_store_sbh(uint16_t opcode)
{
    const auto ro = (opcode >> 6) & 0x7;
    const auto rb = (opcode >> 3) & 0x7;
    const auto rd = opcode & 0x7;

    const auto addr = regs[rb] + regs[ro];

    switch(OP)
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

template<const int OP>
void Cpu::thumb_load_store_reg(uint16_t opcode)
{
    const auto ro = (opcode >> 6) & 0x7;
    const auto rb = (opcode >> 3) & 0x7;
    const auto rd = opcode & 0x7;

    const auto addr = regs[rb] + regs[ro];

    switch(OP)
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

template<const int L>
void Cpu::thumb_load_store_half(uint16_t opcode)
{
    const auto nn = ((opcode >> 6) & 0x1f) * 2;
    const auto rb = (opcode >> 3) & 0x7;
    const auto rd = opcode & 0x7;

    if constexpr(L) // ldrh
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

template<const bool POP, const bool IS_LR>
void Cpu::thumb_push_pop(uint16_t opcode)
{
    const uint8_t reg_range = opcode & 0xff;

    // todo (emtpy r list timings here)
    if constexpr(POP)
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
        if constexpr(IS_LR)
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


        if constexpr(IS_LR) 
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

        if constexpr(IS_LR)
        {
            mem.write_memt<uint32_t>(addr,regs[LR]);
        }
    }

}

template<const int OP>
void Cpu::thumb_hi_reg_ops(uint16_t opcode)
{
    auto rd = opcode & 0x7;
    auto rs = (opcode >> 3) & 0x7;

    // can be used as bl/blx flag (not revlant for gba?)
    const bool msbd = is_set(opcode,7);

    // bit 7 and 6 act as top bits of the reg
    rd = msbd? set_bit(rd,3) : rd;
    rs = is_set(opcode,6)? set_bit(rs,3) : rs;

    const auto rs_val = regs[rs];
    const auto rd_val = regs[rd];

    // only cmp sets flags here!
    switch(OP)
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

template<const int RB, const bool L>
void Cpu::thumb_multiple_load_store(uint16_t opcode)
{
    const auto reg_range = opcode & 0xff;


    // empty r list store pc, sb += 0x40
    if(reg_range == 0)
    {
        // ldmia
        if constexpr(L)
        {
            const auto v = mem.read_memt<uint32_t>(regs[RB]);
            internal_cycle();
            write_pc(v);
        }

        //stmia
        else
        {
            mem.write_memt<uint32_t>(regs[RB],regs[PC]);
        }

        regs[RB] += 0x40;        
    }   

    else
    {
        for(int i = 0; i < 8; i++)
        {
            if(is_set(reg_range,i))
            {
                // ldmia
                if constexpr(L)
                {
                    regs[i] = mem.read_memt<uint32_t>(regs[RB]);
                }
                //stmia
                else
                {
                    mem.write_memt<uint32_t>(regs[RB],regs[i]);
                }
                regs[RB] += ARM_WORD_SIZE;
            }
        }
    }

    
    // one final internal cycle for loads
    if constexpr(L)
    {
        internal_cycle();
    }
}

template<const int OP>
void Cpu::thumb_ldst_imm(uint16_t opcode)
{
    const auto imm = (opcode >> 6) & 0x1f;
    const auto rb = (opcode >> 3) & 0x7;
    const auto rd = opcode & 0x7;

    // 1s + 1n + 1i for ldr
    // 2n for str
    switch(OP)
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

template<const int OP>
void Cpu::thumb_add_sub(uint16_t opcode)
{    
    const auto rd = opcode & 0x7;
    const auto rs = (opcode >> 3) & 0x7;
    const auto rn = (opcode >> 6) & 0x7; // can also be 3 bit imm

    switch(OP)
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

template<const bool FIRST>
void Cpu::thumb_long_bl(uint16_t opcode)
{
    int32_t offset = opcode & 0x7ff; // offset is 11 bits

    if constexpr(FIRST)
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

template<const int TYPE>
void Cpu::thumb_mov_reg_shift(uint16_t opcode)
{
    const auto rd = opcode & 0x7;
    const auto rs = (opcode >> 3) & 0x7;
    const auto n = (opcode >> 6) & 0x1f;

    const auto type = static_cast<shift_type>(TYPE);

    regs[rd] = barrel_shift(type,regs[rs],n,flag_c,true);

    set_nz_flag(regs[rd]);
}

template<const int OP, const int RD>
void Cpu::thumb_mcas_imm(uint16_t opcode)
{
    const uint8_t imm = opcode & 0xff;

    switch(OP)
    {
        case 0b00: // mov
        {
            regs[RD] = imm;
            set_nz_flag(regs[RD]);            
            break;
        }

        case 0b01: //cmp
        {
            sub(regs[RD],imm,true);
            break;
        }

        case 0b10: // add
        {
            regs[RD] = add(regs[RD],imm,true);
            break;
        }

        case 0b11: // sub
        {
            regs[RD] = sub(regs[RD],imm,true);
            break;
        }
    }
}

template<const int COND>
void Cpu::thumb_cond_branch(uint16_t opcode)
{
    // 1st cycle is branch calc overlayed with pipeline
    const int8_t offset = opcode & 0xff;
    const uint32_t addr = regs[PC] + offset*2;

    if(cond_met(COND))
    {
        write_pc(addr);  
    }
}

template<const int RD>
void Cpu::thumb_ldr_pc(uint16_t opcode)
{
    // 0 - 1020 in offsets of 4
    const uint32_t offset = (opcode & 0xff) * 4;

    // pc will have bit two deset to ensure word alignment
    // pc is + 4 ahead of current instr
    const uint32_t addr = (regs[PC] & ~2) + offset;

    regs[RD] = mem.read_memt<uint32_t>(addr);

    // internal cycle for load writeback
    internal_cycle();
}

}
