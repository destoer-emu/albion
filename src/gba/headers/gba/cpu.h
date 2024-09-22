#pragma once
#include <albion/lib.h>
#include <albion/debug.h>
#include <gba/forward_def.h>
#include <gba/cpu_io.h>
#include <gba/arm.h>
#include <gba/dma.h>
#include <gba/interrupt.h>
#include <gba/scheduler.h>


namespace gameboyadvance
{

// tests if a cond field in an instr has been met
constexpr bool cond_lut_helper(u32 cond, u32 flags)
{
    const auto ac = static_cast<arm_cond>(cond);

    const bool flag_z = flags & 1;
    const bool flag_c = flags & 2;
    const bool flag_n = flags & 4;
    const bool flag_v = flags & 8;

    // switch on the cond bits
    // (lower 4)
    switch(ac)
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



using ARM_OPCODE_FPTR = void (Cpu::*)(u32 opcode);
using ARM_OPCODE_LUT = std::array<ARM_OPCODE_FPTR,4096>;

using THUMB_OPCODE_FPTR = void (Cpu::*)(u16 opcode);
using THUMB_OPCODE_LUT = std::array<THUMB_OPCODE_FPTR,1024>;

struct Cpu final
{
    Cpu(GBA &gba);
    void init();
    void log_regs();

    void update_intr_status();

    void handle_power_state();

    void exec_instr_no_debug();

    bool interrupt_ready() const
    {
        return interrupt_service && !is_set(cpsr,7);    
    }

    void do_interrupts()
    {
        if(interrupt_ready())
        {
            service_interrupt();
        }
    }


    using EXEC_INSTR_FPTR = void (Cpu::*)(void);
#ifdef DEBUG
    

    EXEC_INSTR_FPTR exec_instr_fptr = &Cpu::exec_instr_no_debug;

    inline void exec_instr()
    {
        std::invoke(exec_instr_fptr,this);
    }

    void exec_instr_debug();

#else 

    inline void exec_instr()
    {
        exec_instr_no_debug();
    }

#endif

    void exec_instr_no_debug_thumb();
    void exec_instr_no_debug_arm();


    void switch_execution_state(bool thumb)
    {
        is_thumb = thumb;
        cpsr = is_thumb? set_bit(cpsr,5) : deset_bit(cpsr,5);
    }


    void cycle_tick(int cycles)
    {
        scheduler.delay_tick(cycles);
    }


    void internal_cycle();


    void tick_timer(int t, int cycles);
    void insert_new_timer_event(int timer);

    u32 get_pipeline_val() const
    {
        return pipeline[1];
    }


    u32 get_cpsr() const 
    {
        return (cpsr & ~0xf0000000) | (flag_z << Z_BIT) | 
        (flag_c << C_BIT) | (flag_n << N_BIT) | (flag_v << V_BIT);
    } 


    // print all registers for debugging
    // if we go with a graphical debugger
    // we need to make ones for each arrayc
    // and return a std::string
    void print_regs();
    
    void execute_arm_opcode(u32 instr);
    void execute_thumb_opcode(u16 instr);
    
    void request_interrupt(interrupt i);

#ifdef DEBUG
    void change_breakpoint_enable(bool enabled) noexcept
    {
        if(enabled)
        {
            exec_instr_fptr = &Cpu::exec_instr_debug;         
        }

        else
        {
            exec_instr_fptr = &Cpu::exec_instr_no_debug; 
        }
    }
#endif


    // cpu io memory
    CpuIo cpu_io;


    void exec_thumb();
    void exec_arm();

    u32 arm_fetch_opcode();
    u16 thumb_fetch_opcode();

    void arm_pipeline_fill();
    void thumb_pipeline_fill();


    // internal impl

    // fetch speed hacks
    void update_fetch_cache();

    u16 fast_thumb_fetch();
    u16 fast_thumb_fetch_mem();
    void fast_thumb_pipeline_fill();

    u32 fast_arm_fetch();
    u32 fast_arm_fetch_mem();
    void fast_arm_pipeline_fill();

    // slow stable versions
    u16 slow_thumb_fetch();
    void slow_thumb_pipeline_fill();

    u32 slow_arm_fetch();
    void slow_arm_pipeline_fill();


    void write_pc_arm(u32 v);
    void write_pc_thumb(u32 v);
    void write_pc(u32 v);



    //arm cpu instructions
    void arm_unknown(u32 opcode);
    
    template<const bool L>
    void arm_branch(u32 opcode);

    template<const bool S,const bool I, const int OP>
    void arm_data_processing(u32 opcode);

    template<const bool MSR, const bool SPSR, const bool I>
    void arm_psr(u32 opcode);

    template<const bool L, const bool W, const bool P, const bool I>
    void arm_single_data_transfer(u32 opcode);


    void arm_branch_and_exchange(u32 opcode);

    template<const bool P, const bool U, const bool I, const bool L, const bool W>
    void arm_hds_data_transfer(u32 opcode);

    template<const bool S, const bool P, const bool U, const bool W, const bool L>
    void arm_block_data_transfer(u32 opcode);

    template<const bool B>
    void arm_swap(u32 opcode);

    template<const bool S, const bool A>
    void arm_mul(u32 opcode);

    template<bool S, bool A, bool U>
    void arm_mull(u32 opcode);

    void arm_swi(u32 opcode);



    // thumb cpu instructions
    void thumb_unknown(u16 opcode);

    template<const int RD>
    void thumb_ldr_pc(u16 opcode);

    template<const int TYPE>
    void thumb_mov_reg_shift(u16 opcode);

    template<const int COND>
    void thumb_cond_branch(u16 opcode);

    template<const int OP, const int RD>
    void thumb_mcas_imm(u16 opcode);

    template<const bool FIRST>
    void thumb_long_bl(u16 opcode);

    template<const int OP>
    void thumb_alu(u16 opcode);

    template<const int OP>
    void thumb_add_sub(u16 opcode);

    template<const int RB, const bool L>
    void thumb_multiple_load_store(u16 opcode);

    template<const int OP>
    void thumb_hi_reg_ops(u16 opcode);

    template<const int OP>
    void thumb_ldst_imm(u16 opcode);

    template<const bool POP, const bool IS_LR>
    void thumb_push_pop(u16 opcode);

    template<const int L>
    void thumb_load_store_half(u16 opcode);

    void thumb_branch(u16 opcode);

    template<const int RD, const bool IS_PC>
    void thumb_get_rel_addr(u16 opcode);

    template<const int OP>
    void thumb_load_store_reg(u16 opcode);

    template<const int OP>
    void thumb_load_store_sbh(u16 opcode);

    void thumb_swi(u16 opcode);

    void thumb_sp_add(u16 opcode);

    template<const int RD, const bool L>
    void thumb_load_store_sp(u16 opcode);





    // tests if a cond field in an instr has been met
    template<const int COND>
    bool cond_met_constexpr()
    {
        const auto ac = static_cast<arm_cond>(COND);

        if constexpr(ac == arm_cond::eq) { return flag_z; }
        else if constexpr(ac ==  arm_cond::ne) { return !flag_z; }
        else if constexpr(ac ==  arm_cond::cs) { return flag_c; }
        else if constexpr(ac ==  arm_cond::cc) { return !flag_c; }
        else if constexpr(ac ==  arm_cond::mi) { return flag_n; }
        else if constexpr(ac ==  arm_cond::pl) { return !flag_n; }
        else if constexpr(ac ==  arm_cond::vs) { return flag_v; }
        else if constexpr(ac ==  arm_cond::vc) { return !flag_v; }
        else if constexpr(ac ==  arm_cond::hi) { return flag_c && !flag_z; }
        else if constexpr(ac ==  arm_cond::ls) { return !flag_c || flag_z; }
        else if constexpr(ac ==  arm_cond::ge) { return flag_n == flag_v; }
        else if constexpr(ac ==  arm_cond::lt) { return flag_n != flag_v; }
        else if constexpr(ac ==  arm_cond::gt) { return !flag_z && flag_n == flag_v; }
        else if constexpr(ac ==  arm_cond::le) { return flag_z || flag_n != flag_v; }
        else if constexpr(ac ==  arm_cond::al) { return true; }
        else if constexpr(ac ==  arm_cond::nv) { return false; }
    }



    // cpu operations eg adds
    template<const bool S>
    u32 add(u32 v1, u32 v2);

    template<const bool S>
    u32 adc(u32 v1, u32 v2);

    template<const bool S>
    u32 bic(u32 v1, u32 v2);

    template<const bool S>
    u32 sub(u32 v1, u32 v2);

    template<const bool S>
    u32 sbc(u32 v1, u32 v2);

    template<const bool S>
    u32 logical_and(u32 v1, u32 v2);

    template<const bool S>
    u32 logical_or(u32 v1, u32 v2);

    template<const bool S>
    u32 logical_eor(u32 v1, u32 v2);


    void do_mul_cycles(u32 mul_operand);

    bool cond_met(u32 cond)
    {
        const u32 flags = flag_z | flag_c << 1 | flag_n << 2 | flag_v << 3;

        return is_set(cond_lut[cond],flags);
    }

    void service_interrupt();

    void write_stack_fd(u32 reg);
    void read_stack_fd(u32 reg);


    void swi(u32 function);

    // timers
    void timer_overflow(int timer);

    // mode switching
    void switch_mode(cpu_mode new_mode);
    void store_registers(cpu_mode mode);
    void load_registers(cpu_mode mode);
    void set_cpsr(u32 v);
    cpu_mode cpu_mode_from_bits(u32 v);

    //flag helpers
    void set_negative_flag(u32 v);
    void set_zero_flag(u32 v);
    void set_nz_flag(u32 v);

    void set_negative_flag_long(uint64_t v);
    void set_zero_flag_long(uint64_t v);
    void set_nz_flag_long(uint64_t v);

    Display &disp;
    Mem &mem;
    Debug &debug;
    Disass &disass;
    Apu &apu;
    GBAScheduler &scheduler;

    // underlying registers

    // registers in the current mode
    // swapped out with backups when a mode switch occurs
    u32 regs[16] = {0};

    // flags
    // combined into cpsr when it is read
    bool flag_z = false;
    bool flag_n = false;
    bool flag_c = false;
    bool flag_v = false;

    bool interrupt_request = false;
    bool interrupt_service = false;


    bool bios_hle_interrupt = false;


    // backup stores
    u32 user_regs[16] = {0};
    u32 pc_actual = 0;
    u32 cpsr = 0; // status reg

    // r8 - r12 banked
    u32 fiq_banked[5] = {0};

    // regs 13 and 14 banked
    u32 hi_banked[5][2] = {0};

    // banked status regs
    u32 status_banked[5] = {0};

    // in arm or thumb mode?
    bool is_thumb = false;

    bool execute_rom = false;

    // instruction state when the currently executed instruction is fetched
    // required to correctly log branches
    bool is_thumb_fetch = false;

    bool dma_in_progress = false;

    // what context is the arm cpu in
    cpu_mode arm_mode = cpu_mode::system;

    // rather than 16 by 16 bool array
    // store are a bitset
    using COND_LUT = std::array<u16,16>;

    constexpr COND_LUT gen_cond_lut()
    {
        COND_LUT arr{};
        for(u32 c = 0; c < 16; c++)
        {
            for(u32 f = 0; f < 16; f++)
            {
                if(cond_lut_helper(c,f))
                {
                    arr[c] = set_bit(arr[c],f);
                }
            }
        }

        return arr;
    }

    const COND_LUT cond_lut = gen_cond_lut();

    bool in_bios = false;

    // cpu pipeline
    u32 pipeline[2] = {0};


    u8 *fetch_ptr = nullptr;
    u32 fetch_mask = 0;
};



}