#include <gba/gba.h>
#include <limits.h>

namespace gameboyadvance
{

Cpu::Cpu(GBA &gba) : disp(gba.disp), mem(gba.mem), debug(gba.debug), 
    disass(gba.disass), apu(gba.apu)
{
    init_opcode_table();
}   

void Cpu::init()
{
    // current registers
    memset(regs,0,sizeof(user_regs));

    // backup stores
    memset(user_regs,0,sizeof(user_regs));

    // r8 - r12 banked
    memset(fiq_banked,0,sizeof(fiq_banked)); 

    // regs 13 and 14 banked
    memset(hi_banked,0,sizeof(hi_banked));

    // banked status regs
    memset(status_banked,0,sizeof(status_banked));



    // setup main cpu state
    is_thumb = false;  // cpu in arm mode

    write_pc(0x08000000); // cartrige reset vector
    regs[LR] = 0x08000000;
    cpsr = 0x1f;
    regs[SP] = 0x03007f00;
    hi_banked[static_cast<int>(cpu_mode::supervisor)][0] = 0x03007FE0;
    hi_banked[static_cast<int>(cpu_mode::irq)][0] = 0x03007FA0;


//  write_pc(0);
    arm_mode = cpu_mode::system;
    switch_mode(cpu_mode::system);

    // flags
    // combined into cpsr when it is read
    flag_z = false;
    flag_n = false;
    flag_c = false;
    flag_n = false;


    cpu_io.init();
}


void Cpu::init_opcode_table()
{
    init_arm_opcode_table();
    init_thumb_opcode_table();
}

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

void Cpu::init_arm_opcode_table()
{
    arm_opcode_table.resize(4096);


    for(int i = 0; i < 4096; i++)
    {
        arm_opcode_table[i] = &Cpu::arm_unknown;
        switch(i >> 10) // bits 27 and 26 of opcode
        {
        
            case 0b00:
            {

                // 001
                if(is_set(i,9)) 
                {
                    int op = (i >> 5) & 0xf;

                    // msr and mrs
                    // ARM.6: PSR Transfer
                    // bit 24-23 must be 10 for this instr 
                    // bit 20 must also be zero
                    
                    // check it ocupies the unused space for
                    //TST,TEQ,CMP,CMN with a S of zero                    
                    if(op >= 0x8 && op <= 0xb && !is_set(i,4))
                    {
                        arm_opcode_table[i] = &Cpu::arm_psr;
                    }

                    //  ARM.5: Data Processing 00 at bit 27
                    // arm data processing immediate
                    else
                    {
                        arm_opcode_table[i] = &Cpu::arm_data_processing;
                    }
                }

                // 000
                else 
                {
                    //ARM.3: Branch and Exchange
                    // bx
                    if(i == 0b000100100001) 
                    {
                       arm_opcode_table[i] = &Cpu::arm_branch_and_exchange;
                    }

                    // this section of the decoding needs improving....
                    else if((i & 0b1001) == 0b1001)
                    {
                        // ARM.7: Multiply and Multiply-Accumulate (MUL,MLA)
                        if(((i >> 6) & 0b111) == 0b000 && (i & 0xf) == 0b1001)
                        {
                            arm_opcode_table[i] = &Cpu::arm_mul;
                        }

                        // ARM.7: Multiply and Multiply-Accumulate (MUL,MLA) (long)
                        else if(((i >> 7) & 0b11) == 0b01 && (i & 0xf) == 0b1001)
                        {
                            arm_opcode_table[i] = &Cpu::arm_mull;                            
                        }
                        

                        // Single Data Swap (SWP)  
                        else if(is_set(i,8) && (i & 0xf) == 0b1001) // bit 24 set
                        {
                           arm_opcode_table[i] = &Cpu::arm_swap;
                        }

                        // ARM.10: Halfword, Doubleword, and Signed Data Transfer
                        //else if()
                        else 
                        {
                            arm_opcode_table[i] = &Cpu::arm_hds_data_transfer;
                        }
                    }

                    // psr or data processing
                    else
                    {
                        int op = (i >> 5) & 0xf;
                        // check it ocupies the unused space for
                        //TST,TEQ,CMP,CMN with a S of zero 
                        // ARM.6: PSR Transfer                   
                        if(op >= 0x8 && op <= 0xb && !is_set(i,4))
                        {
                            arm_opcode_table[i] = &Cpu::arm_psr;
                        }

                        //  ARM.5: Data Processing 00 at bit 27
                        // arm data processing register
                        else
                        {
                            arm_opcode_table[i] = &Cpu::arm_data_processing;
                        } 
                    }                   
                }
                break;
            }
        

        
            case 0b01:
            {
                //ARM.9: Single Data Transfer
                if(true) // assume for now
                {
                    arm_opcode_table[i] = &Cpu::arm_single_data_transfer;   
                }

                else 
                {
                    arm_opcode_table[i] = &Cpu::arm_unknown;
                }
                break;
            }
        
            case 0b10:
            {

                // 101 (ARM.4: Branch and Branch with Link)
                if(is_set(i,9))
                {
                    arm_opcode_table[i] = &Cpu::arm_branch;
                }

                
                // 100
                // ARM.11: Block Data Transfer (LDM,STM)
                else if(!is_set(i,9))
                {
                    arm_opcode_table[i] = &Cpu::arm_block_data_transfer;
                }
                
                break;
            }
            

            
            case 0b11:
            {

                // 1111 SWI
                if(((i >> 8) & 0b1111) == 0b1111)
                {
                    arm_opcode_table[i] = &Cpu::arm_swi;
                }

                // rest are coprocesor instrucitons and are undefined on the gba
                else
                {
                    arm_opcode_table[i] = &Cpu::arm_unknown;
                }
                break;
            }
            
        } 
    }

}

void Cpu::cycle_tick(int cycles)
{

    UNUSED(cycles);
    // hack until we fix timings
    //cycles = 1;

    disp.tick(cycles);
    apu.tick(cycles);
    tick_timers(cycles);

}


// prefetch buffer stuff should happen in here
void Cpu::internal_cycle()
{
    cycle_tick(1);
}

// should have a list of active timers that aernt count up
// so we aernt needlesly checking them, allthough this is kind of defetaed by a scheduler
// we should also cache the limit and scale on the reg write!
void Cpu::tick_timers(int cycles)
{
    // for each timer
    for(int i = 0; i < 4; i++)
    {        
        auto &timer = cpu_io.timers[i];

        // increments on prev timer overflow so we dont care
        if(timer.count_up)
        {
            continue;
        }

        // timer is not enabled we dont care
        if(!timer.enable)
        {
            continue;
        }

        timer.cycle_count += cycles;

        const auto limit = timer.cycle_limit[timer.scale];

        // if its above the threshold
        if(timer.cycle_count >= limit)
        {
            //puts("timer inc");

            // timer += how many limits passed
            // / (compilier is not smart enough to use a shift here)
            const auto ticks =  timer.cycle_count >> timer.shift_table[timer.scale];

            // adjust cycle count accordingly
            // % (compilier is not smart enough to use a & here)
            timer.cycle_count &= limit - 1;

            // timer overflowed
            if(timer.counter + ticks > 0xffff)
            {
                timer_overflow(i);
            }

            else
            {
                timer.counter += ticks;
            }
        }
    }
}

void Cpu::timer_overflow(int timer_num)
{
    auto &timer = cpu_io.timers[timer_num];

    // reload the timer with inital value
    timer.counter = timer.reload;

    // if fire irq on timer overflow
    if(timer.irq) 
    {
        request_interrupt(timer.timer_interrupt);
    }


    // do we need to fire it specifically to the dma is actually pointing
    // at what fifo reg?
    // need to test what happens on hardware if dmas fire when they dont point to valid regs
    

    // if the timer num is equal to the dma sound channels dma
    // request a fifo dma if it doesent have 16 bytes
    // then push a fifo byte to the apu
    if(timer_num == apu.apu_io.sound_cnt.timer_num_a)
    {
        if(apu.apu_io.fifo_a.len <= 16)
        {
            mem.dma.handle_dma(dma_type::fifo_a);
        }

        const auto x = apu.apu_io.fifo_a.read();
        //printf("fifo a %x\n",x);
        apu.push_dma_a(x);
    }

    if(timer_num == apu.apu_io.sound_cnt.timer_num_b)
    {
        if(apu.apu_io.fifo_b.len <= 16)
        {
            mem.dma.handle_dma(dma_type::fifo_b);
        }

        const auto x = apu.apu_io.fifo_b.read();
        //printf("fifo b %x\n",x);
        apu.push_dma_b(x);
    }


    // check if the timer above is subject to cascade
    // in what oreder should this happen 
    // should the current timer fire its irq first?
    if(timer_num != 3) // timer 0 cant cascade
    {
        auto &next_timer = cpu_io.timers[timer_num+1];

        if(next_timer.enable && next_timer.count_up)
        {
            // about to converflow
            if(next_timer.counter == 0xffff)
            {
                timer_overflow(timer_num+1);
            }

            else
            {
                next_timer.counter += 1;
            }
        }
    }
}



void Cpu::exec_instr_no_debug()
{

    handle_power_state();
    do_interrupts(); 

    // step the cpu in thumb mode
    if(is_thumb) 
    {
        exec_thumb();
    }

     // step the cpu in arm mode
    else
    {
        exec_arm();
    }  
}

#ifdef DEBUG
void Cpu::exec_instr_debug()
{
    const uint32_t pc = pc_actual;
    const uint32_t v = pipeline[0];
	if(debug.breakpoint_hit(pc,v,break_type::execute))
	{
		// halt until told otherwhise :)
		write_log(debug,"[DEBUG] execute breakpoint hit ({:x}:{:x})",pc,v);
		debug.halt();
        return;
	}
    exec_instr_no_debug();
}
#endif

// we need to replace this with proper scheduling but its fine for now
void Cpu::handle_power_state()
{
        // check halt and stop here?
    switch(cpu_io.halt_cnt.state)
    {
        // normal (just run as normal)
        case HaltCnt::power_state::normal:
        {
            break;
        }

        case HaltCnt::power_state::halt:
        {
            //puts("halt");

            // need a better check here to prevent the emulator just locking up
            if(!cpu_io.interrupt_enable)
            {
                throw std::runtime_error(fmt::format("[halt] halt locked up at {:08x}",pc_actual));
            }

            // tick cycles until we an interrupt fires
            // this is gonna be slow as hell until we get a event system
            while(!(cpu_io.interrupt_flag & cpu_io.interrupt_enable))
            {
                cycle_tick(1);
            }
            cpu_io.halt_cnt.state = HaltCnt::power_state::normal;
            break;
        }

        // i think this halts actions until buttons are pressed?
        case HaltCnt::power_state::stop:
        {
            puts("stop unimplemented!");
            cpu_io.halt_cnt.state = HaltCnt::power_state::normal;
            break;
        }
    }
}


// start here
// debug register printing
void Cpu::print_regs()
{

    // update current registers
    // so they can be printed
    store_registers(arm_mode);


    printf("current mode: %s\n",mode_names[static_cast<int>(arm_mode)]);
    printf("cpu state: %s\n", is_thumb? "thumb" : "arm");

    puts("USER & SYSTEM REGS");

    for(int i = 0; i < 16; i++)
    {
        printf("%s: %08x ",user_regs_names[i],user_regs[i]);
        if((i % 2) == 0)
        {
            putchar('\n');
        }
    }


    puts("\n\nFIQ BANKED");


    for(int i = 0; i < 5; i++)
    {
        printf("%s: %08x ",fiq_banked_names[i],fiq_banked[i]);
        if((i % 2) == 0)
        {
            putchar('\n');
        }        
    }

    puts("\nHI BANKED");

    for(int i = 0; i < 5; i++)
    {
        printf("%s: %08x %s: %08x\n",hi_banked_names[i][0],hi_banked[i][0],
            hi_banked_names[i][1],hi_banked[i][1]);
    }

    puts("\nSTAUS BANKED");

    for(int i = 0; i < 5; i++)
    {
        printf("%s: %08x ",status_banked_names[i],status_banked[i]);
        if((i % 2) == 0)
        {
            putchar('\n');
        }       
    }


    printf("\ncpsr: %08x\n",get_cpsr());

    puts("FLAGS");

    printf("Z: %s\n",flag_z? "true" : "false");
    printf("C: %s\n",flag_c? "true" : "false");
    printf("N: %s\n",flag_n? "true" : "false");
    printf("V: %s\n",flag_v? "true" : "false");

}


// set zero flag based on arg
void Cpu::set_zero_flag(uint32_t v)
{
   flag_z =  v == 0;
}


void Cpu::set_negative_flag(uint32_t v)
{
    flag_n = is_set(v,31);
}


// both are set together commonly
// so add a shortcut
void Cpu::set_nz_flag(uint32_t v)
{
    set_zero_flag(v);
    set_negative_flag(v);
}




// set zero flag based on arg
void Cpu::set_zero_flag_long(uint64_t v)
{
    flag_z = v == 0;
}


void Cpu::set_negative_flag_long(uint64_t v)
{
    flag_n = is_set(v,63);  
}


// both are set together commonly
// so add a shortcut
void Cpu::set_nz_flag_long(uint64_t v)
{
    set_zero_flag_long(v);
    set_negative_flag_long(v);
}




void Cpu::switch_mode(cpu_mode new_mode)
{
    // save and load regs
    store_registers(arm_mode);
    load_registers(new_mode);
    arm_mode = new_mode; // finally change modes
    
    // set mode bits in cpsr
    cpsr &= ~0b11111; // mask bottom 5 bits
    cpsr |= get_cpsr_mode_bits(arm_mode);
}


void Cpu::load_registers(cpu_mode mode)
{
    int idx = static_cast<int>(mode);

    switch(mode)
    {

        case cpu_mode::system:
        case cpu_mode::user:
        {
            // load user registers back into registers
            memcpy(regs,user_regs,sizeof(uint32_t) * 15);
            break;
        }


        case cpu_mode::fiq:
        {
            // load bottom 8 user regs
            memcpy(regs,user_regs,sizeof(uint32_t)*8);

            // load fiq banked 
            memcpy(&regs[8],fiq_banked,sizeof(uint32_t)*5);
            regs[SP] = hi_banked[idx][0];
            regs[LR] = hi_banked[idx][1];

            break;
        }

        // they all have the same register layout
        // bar the banked ones
        case cpu_mode::supervisor:
        case cpu_mode::abort:
        case cpu_mode::irq:
        case cpu_mode::undefined:
        {
            // load first 13 user regs back to reg
            memcpy(regs,user_regs,sizeof(uint32_t)*13);

            // load hi regs
            regs[SP] = hi_banked[idx][0];
            regs[LR] = hi_banked[idx][1];


            break;          
        }


        default:
        {
            auto err = fmt::format("[load-regs {:08x}]unhandled mode switch: {:x}\n",pc_actual,idx);
            throw std::runtime_error(err);
        }
    }
}


void Cpu::set_cpsr(uint32_t v)
{
    cpsr = v;

    flag_z = is_set(v,Z_BIT);
    flag_c = is_set(v,C_BIT);
    flag_n = is_set(v,N_BIT);
    flag_v = is_set(v,V_BIT);

    // confirm this?
    is_thumb = is_set(cpsr,5);
    cpu_mode new_mode = cpu_mode_from_bits(cpsr & 0b11111);
    switch_mode(new_mode);    
}

// store current active registers back into the copies
void Cpu::store_registers(cpu_mode mode)
{
    int idx = static_cast<int>(mode);

    switch(mode)
    {

        case cpu_mode::system:
        case cpu_mode::user:
        {
            // store user registers back into registers
            memcpy(user_regs,regs,sizeof(uint32_t) * 15);
            break;
        }


        case cpu_mode::fiq:
        {
            // store bottom 8 user regs
            memcpy(user_regs,regs,sizeof(uint32_t)*8);


            // store fiq banked 
            memcpy(fiq_banked,&regs[8],sizeof(uint32_t)*5);
            hi_banked[idx][0] = regs[SP];
            hi_banked[idx][1] = regs[LR];

            break;
        }

        // they all have the same register layout
        // bar the banked ones
        case cpu_mode::supervisor:
        case cpu_mode::abort:
        case cpu_mode::irq:
        case cpu_mode::undefined:
        {
            // write back first 13 regs to user
            memcpy(user_regs,regs,sizeof(uint32_t)*13);


            // store hi regs
            hi_banked[idx][0] = regs[SP];
            hi_banked[idx][1] = regs[LR];

            break;          
        }


        default:
        {
            auto err = fmt::format("[store-regs {:08x}]unhandled mode switch: {:x}\n",pc_actual,idx);
            throw std::runtime_error(err);
        }
    }
}


cpu_mode Cpu::cpu_mode_from_bits(uint32_t v)
{
    switch(v)
    {
        case 0b10000: return cpu_mode::user;
        case 0b10001: return cpu_mode::fiq;
        case 0b10010: return cpu_mode::irq;
        case 0b10011: return cpu_mode::supervisor;
        case 0b10111: return cpu_mode::abort;
        case 0b11011: return cpu_mode::undefined;
        case 0b11111: return cpu_mode::system;
    }

    // clearly no program should attempt this 
    // but is their a defined behavior for it?
    auto err = fmt::format("unknown mode from bits: {:08x}:{:08x}\n",v,pc_actual);
    throw std::runtime_error(err);
}


// tests if a cond field in an instr has been met
// change this to a lut later
bool Cpu::cond_met(int opcode)
{

    // switch on the cond bits
    // (lower 4)
    switch(static_cast<arm_cond>(opcode & 0xf))
    {
        // z set
        case arm_cond::eq: return flag_z;
        
        // z clear
        case arm_cond::ne: return !flag_z;

        // c set
        case arm_cond::cs: return flag_c;

        // c clear
        case arm_cond::cc: return !flag_c;

        // n set
        case arm_cond::mi: return flag_n;

        // n clear
        case arm_cond::pl: return !flag_n;

        // v set
        case arm_cond::vs: return flag_v; 

        // v clear
        case arm_cond::vc: return !flag_v;

        // c set and z clear
        case arm_cond::hi: return flag_c && !flag_z;

        // c clear or z set
        case arm_cond::ls: return !flag_c || flag_z;

        // n equals v
        case arm_cond::ge: return flag_n == flag_v;

        // n not equal to v
        case arm_cond::lt: return flag_n != flag_v; 

        // z clear and N equals v
        case arm_cond::gt: return !flag_z && flag_n == flag_v;

        // z set or n not equal to v
        case arm_cond::le: return flag_z || flag_n != flag_v;

        // allways
        case arm_cond::al: return true;

        // not valid - see cond_invalid.gba
        case arm_cond::nv: return false;

    }
    return true; // shoud not be reached
}

// common arithmetic and logical operations


/*
thanks yaed for suggesting use of compilier builtins
*/


// this checks if the msb (sign) changes to something it shouldunt
// during arithmetic
template <typename T,typename U, typename X>
inline bool did_overflow(T v1, U v2, X ans) noexcept
{
    return  is_set((v1 ^ ans) & (v2 ^ ans),(sizeof(T)*8)-1); 
}

template <typename T>
inline bool sub_overflow(T v1,T v2) noexcept
{
    if constexpr(std::is_signed<T>())
    {
#ifdef _MSC_VER
        const T ans = v1 - v2;
        // negate 2nd operand so we can pretend
        // this is like an additon
	    return did_overflow(v1,~v2, ans);
#else
        return __builtin_sub_overflow_p(v1,v2,v1);
#endif
    }

    else
    {
        // on arm the the carry flag is set if there is no borrow
        // this is different to x86 so we cant use builtins here
        return v1 >= v2;
    }
}


template <typename T>
inline bool add_overflow(T v1,T v2) noexcept
{
#ifdef _MSC_VER
	const T ans = v1 + v2;
    if constexpr(std::is_signed<T>())
    {
	    return did_overflow(v1, v2, ans);
    }

    else
    {
        return ans < v1;
    }
#else
    return __builtin_add_overflow_p(v1,v2,v1);
#endif
}


uint32_t Cpu::add(uint32_t v1, uint32_t v2, bool s)
{
    const uint32_t ans = v1 + v2;
    if(s)
    {

        flag_v = add_overflow((int32_t)v1,(int32_t)v2);
        flag_c = add_overflow(v1,v2); 

        set_nz_flag(ans);
    }

    return ans;
}


uint32_t Cpu::adc(uint32_t v1, uint32_t v2, bool s)
{

    const uint32_t v3 = flag_c;

    const uint32_t ans = v1 + v2 + v3;

    if(s)
    {

        // ^ as if both operations generate an inproper result we will get an expected sign
        const int32_t ans_signed = v1 + v2;
        flag_v = add_overflow((int32_t)v1,(int32_t)v2) ^ add_overflow(ans_signed,(int32_t)v3);

        // if either operation overflows we need to set the carry
        const uint32_t ans_unsigned = v1 + v2;
        flag_c = add_overflow(v1,v2) || add_overflow(ans_unsigned,v3);

        set_nz_flag(ans);
    }

    return ans;
}


uint32_t Cpu::sub(uint32_t v1, uint32_t v2, bool s)
{
    
    const uint32_t ans = v1 - v2;

    if(s)
    {
        flag_v = sub_overflow((int32_t)v1,(int32_t)v2);
        flag_c = sub_overflow(v1,v2);

        set_nz_flag(ans);
    }


    return ans;
}

uint32_t Cpu::sbc(uint32_t v1, uint32_t v2, bool s)
{
    // subtract one from ans if carry is not set
    const uint32_t v3 = !flag_c;

    const uint32_t ans = v1 - v2 - v3;
    if(s)
    {
        // ^ as if both operations generate an inproper result we will get an expected sign
        const int32_t ans_signed = v1 - v2;
        flag_v = sub_overflow((int32_t)v1,(int32_t)v2) ^ sub_overflow(ans_signed,(int32_t)v3);

        // if both operations overflow we need to set the carry
        const uint32_t ans_unsigned = v1 - v2;
        flag_c = sub_overflow(v1,v2) && sub_overflow(ans_unsigned,v3);

        set_nz_flag(ans);
    }

    return ans;
}


uint32_t Cpu::logical_and(uint32_t v1, uint32_t v2, bool s)
{
    const uint32_t ans = v1 & v2;

    if(s)
    {
        set_nz_flag(ans);
    }

    return ans;
}

uint32_t Cpu::logical_or(uint32_t v1, uint32_t v2, bool s)
{
    const uint32_t ans = v1 | v2;
    if(s)
    {
        set_nz_flag(ans);
    }
    return ans;
}

uint32_t Cpu::bic(uint32_t v1, uint32_t v2, bool s)
{
    const uint32_t ans = v1 & ~v2;
    if(s)
    {
        set_nz_flag(ans);
    }
    return ans;
}

uint32_t Cpu::logical_eor(uint32_t v1, uint32_t v2, bool s)
{
    const uint32_t ans = v1 ^ v2;
    if(s)
    {
        set_nz_flag(ans);
    }
    return ans;
}


/*
https://developer.arm.com/documentation/ddi0210/c/Instruction-Cycle-Timings/Instruction-speed-summary?lang=en
m is:

    1 if bits [31:8] of the multiplier operand are all zero or one, else

    2 if bits [31:16] of the multiplier operand are all zero or one, else

    3 if bits [31:24] of the multiplier operand are all zero or all one, else

    4. 
*/
// all cycles from this are internal
void Cpu::do_mul_cycles(uint32_t mul_operand)
{

    auto cycles = 4;

    if((mul_operand & 0xffffff00) == 0 || (mul_operand & 0xffffff00) == 0xffffff00)
    {
        cycles = 1;
    } 

    else if((mul_operand & 0xffff0000) == 0 || (mul_operand & 0xffff0000) == 0xffff0000)
    {
        cycles = 2;
    } 

    else if((mul_operand & 0xff000000) == 0 || (mul_operand & 0xff000000) == 0xff000000)
    {  
        cycles = 3;
    } 


    for(int i = 0; i < cycles; i++)
    {
        internal_cycle();
    }
}




// write the interrupt req bit
void Cpu::request_interrupt(interrupt i)
{
    cpu_io.interrupt_flag = set_bit(cpu_io.interrupt_flag,static_cast<int>(i));   
}


void Cpu::do_interrupts()
{
    if(is_set(cpsr,7)) // irqs maksed
    {
        return;
    }

    // the handler will find out what fired for us!
    if((cpu_io.interrupt_flag & cpu_io.interrupt_enable) != 0 && cpu_io.ime)
    {
        service_interrupt();
    }
}

// do we need to indicate the interrupt somewhere?
// or does the handler check if?
void Cpu::service_interrupt()
{
    const auto idx = static_cast<int>(cpu_mode::irq);

    // spsr for irq = cpsr
    status_banked[idx] = get_cpsr();

    // lr is next instr + 4 for an irq 
    hi_banked[idx][1] = pc_actual + 4;

    // irq mode switch
    switch_mode(cpu_mode::irq);

    
    // switch to arm mode
    is_thumb = false; // switch to arm mode
    cpsr = deset_bit(cpsr,5); // toggle thumb in cpsr
    cpsr = set_bit(cpsr,7); //set the irq bit to mask interrupts

    write_log(debug,"[irq {:08x}] interrupt flag: {:02x} ",pc_actual,cpu_io.interrupt_flag);

    //internal_cycle();

    write_pc(0x18); // irq handler    
}

void Cpu::write_pc(uint32_t v)
{
    if(is_thumb)
    {
        pc_actual = v & ~1;
        write_pc_thumb(v);
    }

    else
    {
        pc_actual = v & ~3;
        write_pc_arm(v);
    } 
    
}

}