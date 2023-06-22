#include <gba/gba.h>
#include <gba/thumb_lut.h>
#include <gba/cpu.inl>


// i need to properly handle pipeline effects
// within thumb mode as well as in arm mode

namespace gameboyadvance
{


// TODO: remove preftech hacks


void Cpu::slow_thumb_pipeline_fill()
{
    pipeline[0] = mem.read_u16(regs[PC]);
    regs[PC] += ARM_HALF_SIZE;
    pipeline[1] = mem.read_u16(regs[PC]);        
}

u16 Cpu::slow_thumb_fetch()
{
    const u16 opcode = pipeline[0];
    pipeline[0] = pipeline[1];

    regs[PC] += ARM_HALF_SIZE; 
    pc_actual += ARM_HALF_SIZE; 

    pipeline[1] = mem.read_u16(regs[PC]);
    
    return opcode;
}



// TODO: make this work with seq and non seq waitstates
u16 Cpu::fast_thumb_fetch_mem()
{
    mem.update_seq(regs[PC]);

    u16 v = 0;

    const u32 offset = regs[PC] & fetch_mask;
    memcpy(&v,&fetch_ptr[offset],sizeof(v));
    mem.open_bus_value = v;

    cycle_tick(mem.sequential? mem.wait_seq_16 : mem.wait_nseq_16);
    return v;
}

// fetch speed hacks

void Cpu::fast_thumb_pipeline_fill()
{
    pipeline[0] = fast_thumb_fetch_mem(); 
    regs[PC] += ARM_HALF_SIZE;
    pipeline[1] = fast_thumb_fetch_mem();          
}



u16 Cpu::fast_thumb_fetch()
{
    const u16 opcode = pipeline[0];
    pipeline[0] = pipeline[1];
    regs[PC] += ARM_HALF_SIZE; 
    pc_actual += ARM_HALF_SIZE; 


    pipeline[1] = fast_thumb_fetch_mem();
    
    return opcode;
}

u16 Cpu::thumb_fetch_opcode()
{
#ifdef FETCH_SPEEDHACK
    return fast_thumb_fetch();
#else 
    return slow_thumb_fetch();
#endif
}

void Cpu::thumb_pipeline_fill()
{
#ifdef FETCH_SPEEDHACK
    fast_thumb_pipeline_fill();
#else
    slow_thumb_pipeline_fill();
#endif
}


void Cpu::write_pc_thumb(u32 v)
{
    regs[PC] = v & ~1;
    thumb_pipeline_fill();
}

void Cpu::exec_thumb()
{
    const auto op = thumb_fetch_opcode();

    execute_thumb_opcode(op);
}

void Cpu::execute_thumb_opcode(u16 instr)
{
    // get the bits that determine the kind of instr it is
    const u32 op = instr >> 6;

    // call the function from our opcode table
    std::invoke(thumb_opcode_table[op],this,instr);  
}



void Cpu::thumb_unknown(u16 opcode)
{
    const u8 op = get_thumb_opcode_bits(opcode);
    auto err = std::format("[cpu-thumb {:08x}] unknown opcode {:04x}:{:x}\n{}\n",regs[PC],opcode,op,disass.disass_thumb(pc_actual));
    debug.trace.print();
    throw std::runtime_error(err);
}

template<const int RD, const bool L>
void Cpu::thumb_load_store_sp(u16 opcode)
{
    const u32 nn = (opcode & 0xff) * 4;
    const auto addr = regs[SP] + nn;

    if constexpr(L)
    {
        regs[RD] = mem.read_u32(addr);
        regs[RD] = rotr(regs[RD],(addr&3)*8);
        internal_cycle(); // internal for writeback       
    }

    else
    {
        mem.write_u32(addr,regs[RD]);
    } 
}


void Cpu::thumb_sp_add(u16 opcode)
{
    const bool u = !is_set(opcode,7);
    const u32 nn = (opcode & 127) * 4;

    regs[SP] += u? nn : -nn; 
}

void Cpu::thumb_swi(u16 opcode)
{

    //printf("swi %08x: %08x\n",read_pc(),opcode);

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
    switch_execution_state(false); // switch to arm mode
    cpsr = set_bit(cpsr,7); //set the irq bit to mask interrupts

    // branch to interrupt vector
    write_pc(0x8);

    //swi(opcode & 0xff);

}

template<const int RD, const bool IS_PC>
void Cpu::thumb_get_rel_addr(u16 opcode)
{
    const u32 offset = (opcode & 0xff) * 4;

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
void Cpu::thumb_load_store_sbh(u16 opcode)
{
    const auto ro = (opcode >> 6) & 0x7;
    const auto rb = (opcode >> 3) & 0x7;
    const auto rd = opcode & 0x7;

    const auto addr = regs[rb] + regs[ro];

    switch(OP)
    {
        case 0: // strh
        {
            mem.write_u16(addr,regs[rd]);
            break;
        }

        case 1: // ldsb
        {
            regs[rd] = sign_extend<u32>(mem.read_u8(addr),8);
            internal_cycle(); // internal cycle for reg writeback
            break;
        }

        case 2: // ldrh
        {
            regs[rd] = mem.read_u16(addr);
            // result rotated right by 8 on arm7 if unaligned 
            regs[rd] = rotr(regs[rd],8*(addr&1)); 
            internal_cycle(); // internal cycle for reg writeback
            break;
        }

        case 3: //ldsh
        {
            if(!(addr & 1)) // is aligned
            {
                regs[rd] = sign_extend<u32>(mem.read_u16(addr),16);
            }

            else // unaligned
            {
                regs[rd] = sign_extend<u32>(mem.read_u8(addr),8);
            }
            internal_cycle(); // internal cycle for reg writeback
            break;
        }        
    }
}

template<const int OP>
void Cpu::thumb_load_store_reg(u16 opcode)
{
    const auto ro = (opcode >> 6) & 0x7;
    const auto rb = (opcode >> 3) & 0x7;
    const auto rd = opcode & 0x7;

    const auto addr = regs[rb] + regs[ro];

    switch(OP)
    {
        case 0: // str
        {
            mem.write_u32(addr,regs[rd]);
            break;
        }

        case 1: //strb
        {
            mem.write_u8(addr,regs[rd]);
            break;
        }

        case 2: // ldr
        {
            regs[rd] = mem.read_u32(addr);
            regs[rd] = rotr(regs[rd],(addr&3)*8);
            internal_cycle(); // for reg writeback
            break;
        }

        case 3: // ldrb
        {
            regs[rd] = mem.read_u8(addr);
            internal_cycle(); // for reg writeback
            break;
        }
    }
}


void Cpu::thumb_branch(u16 opcode)
{
    const auto offset = sign_extend<int32_t>(opcode & 0x7ff,11) * 2;
    
    const auto old = pc_actual-2;

    if(old == pc_actual)
    {
        while(!interrupt_ready())
        {
            if(scheduler.size() == 0)
            {
                throw std::runtime_error("thumb branch infinite loop");
            }

            scheduler.skip_to_event();
        }   

        write_pc(regs[PC] + offset);            
    }

    else
    {
        write_pc(regs[PC] + offset);
    }
}

template<const int L>
void Cpu::thumb_load_store_half(u16 opcode)
{
    const auto nn = ((opcode >> 6) & 0x1f) * 2;
    const auto rb = (opcode >> 3) & 0x7;
    const auto rd = opcode & 0x7;

    if constexpr(L) // ldrh
    {
        const auto addr = regs[rb] + nn;
        regs[rd] = mem.read_u16(addr);
        // arm7 rotate by 8 if unaligned
        regs[rd] = rotr(regs[rd],8*(addr&1)); 
        internal_cycle(); // internal cycle for writeback
    }   

    else //strh
    {
        mem.write_u16(regs[rb]+nn,regs[rd]);
    } 
}

template<const bool POP, const bool IS_LR>
void Cpu::thumb_push_pop(u16 opcode)
{
    const u8 reg_range = opcode & 0xff;

    // todo (emtpy r list timings here)
    if constexpr(POP)
    {
        for(int i = 0; i < 8; i++)
        {
            if(is_set(reg_range,i))
            {
                regs[i] = mem.read_u32(regs[SP]);
                regs[SP] += ARM_WORD_SIZE;
            }
        }

        // final internal cycle for load
        internal_cycle();

        // nS +1N +1I (pop) | (n+1)S +2N +1I(pop pc)
        if constexpr(IS_LR)
        {
            write_pc(mem.read_u32(regs[SP]));
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

        u32 addr = regs[SP];
        for(int i = 0; i < 8; i++)
        {
            if(is_set(reg_range,i))
            {
                mem.write_u32(addr,regs[i]);
                addr += ARM_WORD_SIZE;
            }
        }

        if constexpr(IS_LR)
        {
            mem.write_u32(addr,regs[LR]);
        }
    }

}

template<const int OP>
void Cpu::thumb_hi_reg_ops(u16 opcode)
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
            regs[rd] = add<false>(rd_val,rs_val);

            if(rd == PC)
            {
                write_pc(regs[PC]);
            }
            break;  
        }

        case 0b01: // cmp
        {
            // do sub and discard result
            sub<true>(rd_val,rs_val);
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
            switch_execution_state(rs_val & 1);


            // branch
            write_pc(rs_val);
            break;
        }
    }
}

template<const int OP>
void Cpu::thumb_alu(u16 opcode)
{
    const auto rs = (opcode >> 3) & 0x7;
    const auto rd = opcode & 0x7;

    switch(OP)
    {

        case 0x0: // and
        {
            regs[rd] = logical_and<true>(regs[rd],regs[rs]);
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
            regs[rd] = adc<true>(regs[rd],regs[rs]);
            break;
        }

        case 0x6: // sbc
        {
            regs[rd] = sbc<true>(regs[rd],regs[rs]);
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
            logical_and<true>(regs[rd],regs[rs]);
            break;
        }

        case 0x9: // neg
        {
            regs[rd] = sub<true>(0,regs[rs]);
            break;
        }

        case 0xa: // cmp
        {
            sub<true>(regs[rd],regs[rs]);
            break;
        }

        case 0xb: // cmn
        {
            add<true>(regs[rd],regs[rs]);
            break;
        }

        case 0xc: // orr
        {
            regs[rd] = logical_or<true>(regs[rd],regs[rs]);
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
void Cpu::thumb_multiple_load_store(u16 opcode)
{
    const auto reg_range = opcode & 0xff;


    // empty r list store pc, sb += 0x40
    if(reg_range == 0)
    {
        // ldmia
        if constexpr(L)
        {
            const auto v = mem.read_u32(regs[RB]);
            internal_cycle();
            write_pc(v);
        }

        //stmia
        else
        {
            mem.write_u32(regs[RB],regs[PC]);
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
                    regs[i] = mem.read_u32(regs[RB]);
                }
                //stmia
                else
                {
                    mem.write_u32(regs[RB],regs[i]);
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
void Cpu::thumb_ldst_imm(u16 opcode)
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
            mem.write_u32((regs[rb]+imm*4),regs[rd]);
            break;
        }

        case 0b01: // ldr
        {
            u32 addr = regs[rb]+imm*4;
            regs[rd] = mem.read_u32(addr);
            regs[rd] = rotr(regs[rd],(addr&3)*8);
            internal_cycle(); // cycle for register writeback
            break;            
        }

        case 0b10: // strb
        {
            mem.write_u8((regs[rb]+imm),regs[rd]);
            break;
        }

        case 0b11: // ldrb
        {
            regs[rd] = mem.read_u8((regs[rb]+imm));
            internal_cycle(); // cycle for register writeback     
            break;
        }
    }
}

template<const int OP>
void Cpu::thumb_add_sub(u16 opcode)
{    
    const auto rd = opcode & 0x7;
    const auto rs = (opcode >> 3) & 0x7;
    const auto rn = (opcode >> 6) & 0x7; // can also be 3 bit imm

    switch(OP)
    {
        case 0b00: // add reg
        { 
            regs[rd] = add<true>(regs[rs],regs[rn]);
            break;
        }
        case 0b01: // sub reg
        { 
            regs[rd] = sub<true>(regs[rs],regs[rn]);
            break;
        }        
        case 0b10: // add imm
        { 
            regs[rd] = add<true>(regs[rs],rn);
            break;
        }        
        case 0b11: // sub imm
        { 
            regs[rd] = sub<true>(regs[rs],rn);
            break;
        }        
    }
}

template<const bool FIRST>
void Cpu::thumb_long_bl(u16 opcode)
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
        const u32 tmp = pc_actual;
        // pc = lr + offsetlow << 1
        write_pc((regs[LR] + (offset << 1)));
        // lr = tmp | 1
        regs[LR] = tmp | 1;
        write_log(debug,"[cpu-thumb {:08x}] call {:08x}",tmp,pc_actual);
        //printf("[%08x] call %08x\n",tmp,pc_actual);
    }
}

template<const int TYPE>
void Cpu::thumb_mov_reg_shift(u16 opcode)
{
    const auto rd = opcode & 0x7;
    const auto rs = (opcode >> 3) & 0x7;
    const auto n = (opcode >> 6) & 0x1f;

    const auto type = static_cast<shift_type>(TYPE);

    regs[rd] = barrel_shift(type,regs[rs],n,flag_c,true);

    set_nz_flag(regs[rd]);
}

template<const int OP, const int RD>
void Cpu::thumb_mcas_imm(u16 opcode)
{
    const u8 imm = opcode & 0xff;

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
            sub<true>(regs[RD],imm);
            break;
        }

        case 0b10: // add
        {
            regs[RD] = add<true>(regs[RD],imm);
            break;
        }

        case 0b11: // sub
        {
            regs[RD] = sub<true>(regs[RD],imm);
            break;
        }
    }
}

template<const int COND>
void Cpu::thumb_cond_branch(u16 opcode)
{
    if(cond_met_constexpr<COND>())
    {
        // 1st cycle is branch calc overlayed with pipeline
        const int8_t offset = opcode & 0xff;
        const u32 addr = regs[PC] + offset*2;
        write_pc(addr);  
    }
}

template<const int RD>
void Cpu::thumb_ldr_pc(u16 opcode)
{
    // 0 - 1020 in offsets of 4
    const u32 offset = (opcode & 0xff) * 4;

    // pc will have bit two deset to ensure word alignment
    // pc is + 4 ahead of current instr
    const u32 addr = (regs[PC] & ~2) + offset;

    regs[RD] = mem.read_u32(addr);

    // internal cycle for load writeback
    internal_cycle();
}

}
