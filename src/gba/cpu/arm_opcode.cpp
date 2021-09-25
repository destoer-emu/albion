#include <gba/gba.h>
#include <gba/arm_lut.h>

namespace gameboyadvance
{

// if there is a pipeline stall (whenever pc changes besides a fetch)
// TODO remove prefetch hacks
void Cpu::arm_fill_pipeline() // need to verify this...
{
    // read_rom will break if the program wraps around to the bios region
    // (this will cause a crash unless the bios swap hardware feature is on)
    // in this case we cant use this code
    if(execute_rom)
    {
        const auto wait = rom_wait_sequential_32;
        pipeline[0] = mem.read_rom<uint32_t>(regs[PC]); cycle_tick(wait);
        regs[PC] += ARM_WORD_SIZE;
        pipeline[1] = mem.read_rom<uint32_t>(regs[PC]); cycle_tick(wait);
    }

    else
    {
        pipeline[0] = mem.read_u32(regs[PC]);
        regs[PC] += ARM_WORD_SIZE;
        pipeline[1] = mem.read_u32(regs[PC]);       
    }
}

void Cpu::write_pc_arm(uint32_t v)
{
    regs[PC] = v & ~3;
    arm_fill_pipeline(); // fill the intitial cpu pipeline
}


uint32_t Cpu::fetch_arm_opcode()
{
    const uint32_t opcode = pipeline[0];
    pipeline[0] = pipeline[1];
    regs[PC] += ARM_WORD_SIZE; 
    pc_actual += ARM_WORD_SIZE;
    if(execute_rom)
    {
        const auto wait = rom_wait_sequential_32;
        pipeline[1] = mem.read_rom<uint32_t>(regs[PC]);
        cycle_tick(wait);
    }

    else
    {   
        pipeline[1] = mem.read_u32(regs[PC]);   
    }
    return opcode;
}

void Cpu::execute_arm_opcode(uint32_t instr)
{
    // get the bits that determine the kind of instr it is
    const auto op = get_arm_opcode_bits(instr);

    // call the function from our opcode table
    std::invoke(arm_opcode_table[op],this,instr);   
}

void Cpu::exec_arm()
{
    const auto instr = fetch_arm_opcode();

    // if the condition is not met just
    // advance past the instr
    if(cond_met((instr >> 28) & 0xf))
    {
        execute_arm_opcode(instr);
    }
}




void Cpu::arm_unknown(uint32_t opcode)
{
    const auto op = ((opcode >> 4) & 0xf) | ((opcode >> 16) & 0xff0);
    const auto err = fmt::format("[cpu-arm {:08x}] unknown opcode {:08x}:{:08x}\n{}\n",pc_actual,opcode,op,disass.disass_arm(get_pc()));
    debug.trace.print();
    throw std::runtime_error(err);
}


void Cpu::log_regs()
{
    for(int i = 0; i < 15; i++)
    {
        printf("%08x ",regs[i]);
    }

    printf("%08x \n",get_pc() + (is_thumb? 2 : 4));
}


void Cpu::arm_swi(uint32_t opcode)
{

    //printf("swi %08x: %08x\n",get_pc(),opcode);

    // nn is ignored by hardware
    UNUSED(opcode);

    const auto idx = static_cast<int>(cpu_mode::supervisor);

    // spsr for supervisor = cpsr
    status_banked[idx] = get_cpsr();

    // lr in supervisor mode set to return addr
    hi_banked[static_cast<int>(idx)][1] = pc_actual;

    // supervisor mode switch
    switch_mode(cpu_mode::supervisor);

    cpsr = set_bit(cpsr,7); //set the irq bit to mask interrupts

    //internal_cycle();

    // branch to interrupt vector
    write_pc(0x8);

    //swi((opcode & 0x00ffffff) >> 16);
}

// mul timings need to be worked on
// double check where internal cycles go here
template<bool S, bool A, bool U>
void Cpu::arm_mull(uint32_t opcode)
{
    const auto rm = opcode & 0xf;
    const auto rs = (opcode >> 8) & 0xf;
    const auto rdhi = (opcode >> 16) & 0xf;
    const auto rdlo = (opcode >> 12) & 0xf;

    uint64_t result;

    if constexpr(U) // unsigned
    {
        uint64_t ans;
        if constexpr(A)
        {
            const uint64_t oper = ((uint64_t)regs[rdhi] << 32) | (uint64_t)regs[rdlo];
            ans = (uint64_t)regs[rs] * (uint64_t)regs[rm] + oper;
            do_mul_cycles(regs[rm]);
            internal_cycle();
        }

        else
        {
            ans = (uint64_t)regs[rs] * (uint64_t)regs[rm];
            do_mul_cycles(regs[rm]);
        }
        result = ans;       
    }

    else // signed
    {
        int64_t ans;
        const auto v1 = sign_extend<int64_t>(regs[rs],32);
        const auto v2 = sign_extend<int64_t>(regs[rm],32);
        if constexpr(A)
        {
            int64_t oper = ((int64_t)regs[rdhi] << 32) | (int64_t)regs[rdlo];
            ans =  v1 * v2 + oper;
            do_mul_cycles(regs[rm]);
            internal_cycle();
        }

        else
        {
            ans = v1 * v2;
            do_mul_cycles(regs[rm]);
        }
        result = (uint64_t)ans;
    }

    // write the ans
    regs[rdhi] = (result >> 32) & 0xffffffff;
    regs[rdlo] = result & 0xffffffff;
    internal_cycle();


    // c destroyed
    if constexpr(S)
    {
        set_nz_flag_long(result);

        // c destroyed
        flag_c = false;
    }
}

// neeeds a more accurate timings fix
template<const bool S, const bool A>
void Cpu::arm_mul(uint32_t opcode)
{
    // first is s cycle from pipeline rest is internal
    // dependant on the input
    const auto rn = (opcode >> 12) & 0xf;
    const auto rd = (opcode >> 16) & 0xf;
    const auto rs = (opcode >> 8) & 0xf;
    const auto rm = opcode & 0xf;

    if constexpr (A) // mla
    {
        regs[rd] = regs[rm] * regs[rs] + regs[rn];
        do_mul_cycles(regs[rs]);
        internal_cycle(); // extra internal cycle for accumulate
    }   

    else // mul
    {
        regs[rd] = regs[rm] * regs[rs];
        do_mul_cycles(regs[rs]);
    }

    if constexpr(S)
    {
        set_nz_flag(regs[rd]);

        // c destroyed
       flag_c = false;
    }
}

template<const bool B>
void Cpu::arm_swap(uint32_t opcode)
{
    const auto rm = opcode & 0xf;
    const auto rd = (opcode >> 12) & 0xf;
    const auto rn = (opcode >> 16) & 0xf;


    // swp works propely even if rm and rn are the same
    uint32_t tmp; 

    // rd = [rn], [rn] = rm
    if constexpr(B)
    {
        tmp = mem.read_u8(regs[rn]);
        mem.write_u8(regs[rn],regs[rm]);
    }

    else
    {
        tmp = mem.read_u32(regs[rn]);
        regs[rd] = rotr(regs[rd],(regs[rn]&3)*8);
        mem.write_u32(regs[rn],regs[rm]);
    }


    regs[rd] = tmp;
    internal_cycle(); // internal for writeback
}

// <--- double check this code as its the most likely error source
// need timings double checked on this
template<const bool S, const bool P, const bool U, const bool W, const bool L>
void Cpu::arm_block_data_transfer(uint32_t opcode)
{
    const auto rn = (opcode >> 16) & 0xf;
    const auto rlist = opcode & 0xffff;



    uint32_t addr = regs[rn];
    uint32_t old_base = regs[rn];
    UNUSED(old_base);

    bool base_writeback_hit = false;

   
    unsigned int first = 0;
#ifdef _MSC_VER
    for(int i = 15; i >= 0; i--)
    {
        if(is_set(rlist,i))
        {
            first = i;
            if(!U)
            {
                addr -= ARM_WORD_SIZE;
            }
        }
    }
#else
    if(!U)
    {
        addr -= __builtin_popcount(rlist) * ARM_WORD_SIZE;
    }
    first = __builtin_ctz(rlist);
#endif 

    const bool has_pc = is_set(rlist,PC);

    // allways adding on address so if  we are in "down mode"
    // we need to precalc the bottom
    if constexpr(!U) 
    {
        if constexpr(W)
        {
            regs[rn] = addr;

            if(rn == PC)
            {
                write_pc(regs[PC]);
            }
            base_writeback_hit = true;
        }
    }


    cpu_mode old_mode = arm_mode;
    bool changed_mode = false;
    if constexpr(S)
    {
        // no r15 or load uses different mode
        // dont bother switching if are in user mode
        if((!has_pc || !L) && arm_mode != cpu_mode::user)
        {
            changed_mode = true;
            switch_mode(cpu_mode::user);
        }
    }


    for(unsigned int i = first; i < 16; i++)
    {
        if(!is_set(rlist,i))
        {
            continue;
        }

        if constexpr(P) 
        {
            addr += ARM_WORD_SIZE;
        }


        if constexpr(L) // load
        {
            // no writeback if base
            if(i == rn)
            {
               base_writeback_hit = true;
            }

            if(i == PC)
            {
                write_pc(mem.read_u32(addr));
            }

            else
            {
                regs[i] = mem.read_u32(addr);
            }


            // if pc is in list and s bit set  cpsr = spsr
            if(i == PC && S)
            {
                const auto idx = static_cast<int>(arm_mode);
                // not in user or system mode
                // actually what happens if we attempt this?
                // just cpsr=cpsr and it does nothing?
                if(arm_mode < cpu_mode::user)  
                {
                    set_cpsr(status_banked[idx]);
                }

                // likely follows standard behavior for msr
                else
                {
                    auto err = fmt::format("[block data: {:08x}] illegal status bank {:x}\n",pc_actual,idx);
                    throw std::runtime_error(err);
                }
                // TODO what happens if thumb bit changed here
            }
        }

        else // store
        {
            // if base is is first entry
            // store old base
            if(rn == i && i == first)
            {
                mem.write_u32(addr,old_base);
            }

            else
            {
                mem.write_u32(addr,regs[i]);
            }
        }

        if constexpr(!P)
        {
            addr += ARM_WORD_SIZE;
        }
    }

    if constexpr(L)
    {
        // internal cycle for last reg writeback
        internal_cycle();
    }

    //writeback higher address if it went up
    // does this have a timing implication!?
    if constexpr(W)
    {
        if(!base_writeback_hit)
        {
            regs[rn] = addr;
            if(rn == PC)
            {
                write_pc(regs[rn]);
            }
        }
    }


    // restore the correct mode!
    if(changed_mode)
    {
        switch_mode(old_mode);
    }
}


// need to handle instr variants and timings on these
template<const bool L>
void Cpu::arm_branch(uint32_t opcode)
{
    //1st cycle calc branch addr (this is overlayed with the initial pipeline)
    // fetch which we have allready done before we have even entered this func
    const auto offset = sign_extend<int32_t>(opcode & 0xffffff,24) << 2;

    // 2nd & 3rd are tken up by a pipeline refill from branch target
    // here if the link bit is set pc is also saved into lr
    if constexpr(L)
    {
        // save addr of next instr
        regs[LR] = (pc_actual) & ~3; // bottom bits deset
    }

    const auto old = pc_actual-4;


    if(old == pc_actual)
    {
        while(!interrupt_ready())
        {
            if(scheduler.size() == 0)
            {
                throw std::runtime_error("arm branch infinite loop");
            }

            scheduler.skip_to_event();
        }

        write_pc(regs[PC] + offset);  
    }

    else
    {
        // should switch to sequential access here
        // writing to the pc will trigger the pipeline refill
        write_pc(regs[PC] + offset);
    }
}

// psr transfer
// TODO handle effects of directly writing the thumb bit 
// double checking timings
template<const bool MSR, const bool SPSR, const bool I>
void Cpu::arm_psr(uint32_t opcode)
{
    // msr
    if constexpr(MSR)
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
        auto v = I? get_arm_operand2_imm(opcode) : regs[opcode & 0xf];

        // only write specifed bits
        v &= mask;
        
        // all bits in mask should be deset
        // and then the value ored
        if constexpr(!SPSR) // cpsr
        {
            const auto psr = get_cpsr();
            set_cpsr((psr & ~mask) | v);
        }

        else // spsr
        {
            if(arm_mode < cpu_mode::user)
            {
                const auto idx = static_cast<int>(arm_mode);
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
        const auto rd = (opcode >> 12) & 0xf;

        if constexpr(SPSR)
        {
            
            if(arm_mode < cpu_mode::user)
            {
                const auto idx = static_cast<int>(arm_mode);
                regs[rd] = status_banked[idx];
            }

            // user and system read cpsr where spsr would normally be read (see starbreezes test)
            else 
            {
                regs[rd] = get_cpsr();
            }
        }

        else
        {
            regs[rd] = get_cpsr();
        }


        if(rd == PC)
        {
            write_pc(regs[PC]);
        }
    }
}

// look what the internal cycles are here
template<const bool S,const bool I, const int OP>
void Cpu::arm_data_processing(uint32_t opcode)
{
    // 1st cycle is handled due to pipeline


    // just slowly impl each opcode here
    // dont do the entire switch at once
    const auto rd = (opcode >> 12) & 0xf;
    const auto rn = (opcode >> 16) & 0xf;

    // default to preserve the carry
    // incase of a zero shift
    bool shift_carry = flag_c;

    uint32_t op1 = regs[rn];
    uint32_t op2;

    // ror shifted immediate in increments of two
    if constexpr(I)
    {
        // how to calc the carry?
        const auto imm = opcode & 0xff;
        const auto shift = ((opcode >> 8) & 0xf)*2;

        if(shift != 0)
        {
            shift_carry = is_set(imm,shift-1);
        }
        
        op2 = rotr(imm,shift);
    }

    else // shifted register 
    {
        const auto type = static_cast<shift_type>((opcode >> 5 ) & 0x3);



        // immediate is allways register rm
        const auto rm = opcode & 0xf;
        uint32_t imm = regs[rm];


        uint32_t shift_ammount = 0;
        // shift ammount is a register
        if(is_set(opcode,4))
        {
            // bottom byte of rs (no r15)
            const auto rs = (opcode >> 8) & 0xf;

            shift_ammount = regs[rs] & 0xff; 

            // if a reg shift ammount is used its +12
            // this is because of the pipeline can we actually observe
            // the difference in when the fetch happens in any meaningful way?
            if(rm == PC)
            {
                imm += 4; 
            }

            if(rn == PC)
            {
                op1 += 4;
            }
            
            // internal cycle for shift by register
            internal_cycle();
        }

        else // shift ammount is an 5 bit int
        {
            shift_ammount = (opcode >> 7) & 0x1f;
        }


        op2 = barrel_shift(type,imm,shift_ammount,shift_carry,!is_set(opcode,4));
    }


    const bool rd_pc = rd == PC;

    // switch on the opcode to decide what to do
    switch(OP)
    {
        case 0x0: //and
        {
            regs[rd] = logical_and<S>(op1,op2);
            if(S)
            {
                flag_c = shift_carry;
            }
            break;
        }

        case 0x1: // eor
        {
            regs[rd] = logical_eor<S>(op1,op2);
            if(S)
            {
                flag_c = shift_carry;
            }
            break;            
        }

        case 0x2: // sub
        {
            regs[rd] = sub<S>(op1,op2);
            break;
        }

        case 0x3: // rsb
        {
            regs[rd] = sub<S>(op2,op1);
            break;
        }

        case 0x4: // add
        {
            regs[rd] = add<S>(op1,op2);
            break;
        }


        case 0x5: // adc
        {
            regs[rd] = adc<S>(op1,op2);
            break;           
        }

        case 0x6: // sbc
        {
            regs[rd] = sbc<S>(op1,op2);
            break;
        }

        case 0x7: // rsc
        {
            regs[rd] = sbc<S>(op2,op1);
            break;
        }

        case 0x8: // tst (and without writeback)
        {
            logical_and<S>(op1,op2);
            if(S)
            {
                flag_c = shift_carry;
            }             
            break;
        }


        case 0x9: // teq
        {
            logical_eor<S>(op1,op2);
            if(S)
            {
                flag_c = shift_carry;
            }            
            break;
        }

        case 0xa: // cmp
        {
            sub<S>(op1,op2);
            break;
        }

        case 0xb: // cmn
        {
            add<S>(op1,op2);
            break;            
        }

        case 0xc: //orr
        {
            regs[rd] = logical_or<S>(op1,op2);
            if(S)
            {
                flag_c = shift_carry;
            }                   
            break;
        }

        case 0xd: // mov
        {
            regs[rd] = op2;
            // V preserved
            if(S) // if the set cond bit is on
            {
                set_nz_flag(regs[rd]);
                // carry is that of the shift oper
                flag_c = shift_carry;                 
            }
            break;
        }


        case 0xe: // bic
        {
            regs[rd] = bic<S>(op1,op2);
            if(S)
            {
                flag_c = shift_carry;
            }
            break;
        }

        case 0xf: // mvn
        {
            regs[rd] = ~op2;
            // V preserved
            if(S) // if the set cond bit is on
            {
                set_nz_flag(regs[rd]);
                // carry is that of the shift oper
                flag_c = shift_carry;                 
            }
            break;
        }
    }

    if(rd_pc)
    {

        if constexpr(S)
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

        if constexpr(!(OP >= 0x8 && OP <= 0xb))
        {
            write_pc(regs[rd]);
        }
    }
    

}


// bx 
void Cpu::arm_branch_and_exchange(uint32_t opcode)
{
    // 1st cycle is address calc overlayed with pipeline fetch
    const auto rn = opcode & 0xf;

    // if bit 0 of rn is a 1
    // subsequent instrs decoded as thumb
    switch_execution_state(regs[rn] & 1);

    write_pc(regs[rn]);
}


// halfword doubleword signed data transfer
// <-- handle instr timings
template<const bool P, const bool U, const bool I, const bool L, const bool W>
void Cpu::arm_hds_data_transfer(uint32_t opcode)
{
    const auto rn = (opcode >> 16) & 0xf;
    const auto rd = (opcode >> 12) & 0xf;
    const auto op = (opcode >> 5) & 0x3;
    



    auto addr = regs[rn];

    uint32_t offset;

    if constexpr(!I) 
    {
        const auto rm = opcode & 0xf;
        offset = regs[rm];
    }

    else
    {
        uint8_t imm = opcode & 0xf;
        imm |= (opcode >> 4) & 0xf0;
        offset = imm;
    }



    if constexpr(P) // pre
    {
        addr += U? offset : -offset;
    }


    // now we choose between load and store
    // and just switch on the opcode
    if constexpr(L) // load
    {
        const bool is_pc = rd == PC;

        switch(op)
        {
            case 0:
            {
                auto err = fmt::format("hds illegal load op: {:08x}:{:08x}\n",pc_actual,opcode);
                throw std::runtime_error(err);
                break;
            }

            case 1: // ldrh
            {
                regs[rd] = mem.read_u16(addr);
                // result rotated right by 8 on arm7 if unaligned 
                regs[rd] = rotr(regs[rd],8*(addr&1)); 
                internal_cycle(); // internal cycle for writeback
                if(is_pc)
                {
                    write_pc(regs[rd]);
                }
                break;
            }

            case 2: // ldrsb
            {
                regs[rd] = sign_extend<uint32_t>(mem.read_u8(addr),8);
                internal_cycle(); // internal cycle for writeback
                if(is_pc)
                {
                    write_pc(regs[rd]);
                }
                break;
            }

            case 3: // ldrsh
            {

                if(!(addr & 1))
                {
                    regs[rd] = sign_extend<uint32_t>(mem.read_u16(addr),16);
                }

                // unaligned
                else
                {
                    regs[rd] = sign_extend<uint32_t>(mem.read_u8(addr),8);
                }



                internal_cycle(); // internal cycle for writeback
                if(is_pc)
                {
                    write_pc(regs[rd]);
                }
                break;
            }
        }
    }

    else // store
    {
        auto value = regs[rd];
        if(rd == PC)
        {
            // pc + 12
            value += 4;
        }

        switch(op)
        {
            case 1: // strh
            {
                mem.write_u16(addr,value);
                break;
            }
            
            default: // doubleword ops not supported on armv4
            {
                auto err = fmt::format("hds illegal store op: {:08x}:{:08x}\n",pc_actual,opcode);
                throw std::runtime_error(err);
            }

        }
    }


    // handle any write backs that occur
    // arm says we cant use the same rd and base with a writeback
    if(rn != rd) 
    {
        if constexpr(!P) // post allways do a writeback
        {
            regs[rn] += U? offset : -offset;

            if(rn == PC)
            {
                write_pc(regs[rn]);
            }
        }

        else if constexpr(W) // writeback
        {
            regs[rn] = addr;

            if(rn == PC)
            {
                write_pc(regs[rn]);
            }
        }
    }
}

// ldr , str
template<const bool L, const bool W, const bool P, const bool I>
void Cpu::arm_single_data_transfer(uint32_t opcode)
{

    // 1st cycle is address calc (overlayed with pipeline)

    // does not do this on gba?
    // does it do this on something like the ds (look at arm manuals)
    //const auto cur_mode = arm_mode;
    //const bool mode_change = !p && w; // T


    const auto rd = (opcode >> 12) & 0xf;
    const auto rn = (opcode >> 16) & 0xf;

    uint32_t offset;

    if constexpr(I)
    {
        const auto type = static_cast<shift_type>((opcode >> 5 ) & 0x3);


        // immediate is allways register rm
        const int rm = opcode & 0xf;
        const auto imm = regs[rm];


        // register specified shift ammounts are not allowed
        const int shift_ammount = (opcode >> 7) & 0x1f;
        
        bool carry = flag_c;
        offset = barrel_shift(type,imm,shift_ammount,carry,true);
    }

    else // immeditate
    {
        // 12 bit immediate
        offset = opcode & 0xfff;
    }


    //byte / word bit
    const bool is_byte = is_set(opcode,22);

    // up or down bit decides wether we add
    // or subtract the offest
    const bool u = is_set(opcode,23);


    auto addr = regs[rn];

    // up / down bit decides wether to add or subtract
    // the offset
    if constexpr(P)
    {
        addr += u? offset : -offset;
    }



    // perform the specifed memory access
    if constexpr(L) // ldr
    {
        // ldrb
        if(is_byte)
        {
            regs[rd] = mem.read_u8(addr);
        }

        else // ldr and swp use rotated reads
        {
            regs[rd] = mem.read_u32(addr);
            regs[rd] = rotr(regs[rd],(addr&3)*8);
        }
       
        // data written back to reg 
        internal_cycle();

        if(rd == PC)
        {
            write_pc(regs[rd]);
        }
    }

    else // str 
    {
        uint32_t v = regs[rd];

        // due to prefetch pc is now higher
        // (pc+12)
        if(rd == PC)
        {
            v += 4;
        }

		if(is_byte)
		{
			mem.write_u8(addr,v);
		}
		
		else
		{
			mem.write_u32(addr,v);
		}
    }


    // handle any write backs that occur
    // arm says we cant use the same rd and base with a writeback
    // but theres impl defined behavior that gba-suite tests here
    // but i dont know what it is
    if(rn != rd) 
    {
        if constexpr(!P) // post allways do a writeback
        {
            regs[rn] += u? offset : -offset;

            if(rn == PC)
            {
                write_pc(regs[rn]);
            }
        }

        else if constexpr(W) // writeback
        {
            regs[rn] = addr;

            if(rn == PC)
            {
                write_pc(regs[rn]);
            }
        }
    }

}

}
