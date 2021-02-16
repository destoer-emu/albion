#pragma once
#include <destoer-emu/lib.h>
#include <destoer-emu/debug.h>
#include <gba/forward_def.h>
#include <gba/cpu_io.h>
#include <gba/arm.h>
#include <gba/dma.h>
#include <gba/interrupt.h>


namespace gameboyadvance
{


class Cpu
{
public:
    Cpu(GBA &gba);
    void init();
    void log_regs();


#ifdef DEBUG
    using EXEC_INSTR_FPTR = void (Cpu::*)(void);

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


    void exec_instr_no_debug();


    void cycle_tick(int cylces); // advance the system state

    void tick_timer(int t, int cycles);
    void insert_new_timer_event(int timer);

    uint32_t get_pipeline_val() const
    {
        return pipeline[1];
    }

    uint32_t get_pc() const 
    {
        return pc_actual;
    }
    uint32_t get_user_regs(int idx) const {return user_regs[idx];}
    uint32_t get_current_regs(int idx) const {return regs[idx]; }
    uint32_t get_status_regs(int idx) const {return status_banked[idx]; }
    uint32_t get_fiq_regs(int idx) const {return fiq_banked[idx]; }
    uint32_t get_high_regs(int idx, int idx2) const {return hi_banked[idx][idx2]; }
    uint32_t get_cpsr() const 
    {
        return (cpsr & ~0xf0000000) | (flag_z << Z_BIT) | 
        (flag_c << C_BIT) | (flag_n << N_BIT) | (flag_v << V_BIT);
    } 


    cpu_mode get_mode() const { return arm_mode; }

    bool is_cpu_thumb() const { return is_thumb; }

    // print all registers for debugging
    // if we go with a graphical debugger
    // we need to make ones for each arrayc
    // and return a std::string
    void print_regs();
    
    void execute_arm_opcode(uint32_t instr);
    void execute_thumb_opcode(uint16_t instr);
    
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
private:

    using ARM_OPCODE_FPTR = void (Cpu::*)(uint32_t opcode);
    using THUMB_OPCODE_FPTR = void (Cpu::*)(uint16_t opcode);
    std::vector<ARM_OPCODE_FPTR> arm_opcode_table;
    std::vector<THUMB_OPCODE_FPTR> thumb_opcode_table;
    void init_opcode_table();
    void init_arm_opcode_table();
    void init_thumb_opcode_table();
    

    void exec_thumb();
    void exec_arm();

    uint32_t fetch_arm_opcode();
    uint16_t fetch_thumb_opcode();

    void arm_fill_pipeline();
    void thumb_fill_pipeline(); 
    void write_pc_arm(uint32_t v);
    void write_pc_thumb(uint32_t v);
    void write_pc(uint32_t v);


    void internal_cycle();

    bool cond_met(int opcode);

    
    void handle_power_state();

    //arm cpu instructions
    void arm_unknown(uint32_t opcode);
    void arm_branch(uint32_t opcode);
    void arm_data_processing(uint32_t opcode);
    void arm_psr(uint32_t opcode);
    void arm_single_data_transfer(uint32_t opcode);
    void arm_branch_and_exchange(uint32_t opcode);
    void arm_hds_data_transfer(uint32_t opcode);
    void arm_block_data_transfer(uint32_t opcode);
    void arm_swap(uint32_t opcode);
    void arm_mul(uint32_t opcode);
    void arm_mull(uint32_t opcode);
    void arm_swi(uint32_t opcode);

    // thumb cpu instructions
    void thumb_unknown(uint16_t opcode);
    void thumb_ldr_pc(uint16_t opcode);
    void thumb_mov_reg_shift(uint16_t opcode);
    void thumb_cond_branch(uint16_t opcode);
    void thumb_mcas_imm(uint16_t opcode);
    void thumb_long_bl(uint16_t opcode);
    void thumb_alu(uint16_t opcode);
    void thumb_add_sub(uint16_t opcode);
    void thumb_multiple_load_store(uint16_t opcode);
    void thumb_hi_reg_ops(uint16_t opcode);
    void thumb_ldst_imm(uint16_t opcode);
    void thumb_push_pop(uint16_t opcode);
    void thumb_load_store_half(uint16_t opcode);
    void thumb_branch(uint16_t opcode);
    void thumb_get_rel_addr(uint16_t opcode);
    void thumb_load_store_reg(uint16_t opcode);
    void thumb_load_store_sbh(uint16_t opcode);
    void thumb_swi(uint16_t opcode);
    void thumb_sp_add(uint16_t opcode);
    void thumb_load_store_sp(uint16_t opcode);

    // cpu operations eg adds
    uint32_t add(uint32_t v1, uint32_t v2, bool s);
    uint32_t adc(uint32_t v1, uint32_t v2, bool s);
    uint32_t bic(uint32_t v1, uint32_t v2, bool s);
    uint32_t sub(uint32_t v1, uint32_t v2, bool s);
    uint32_t sbc(uint32_t v1, uint32_t v2, bool s);
    uint32_t logical_and(uint32_t v1, uint32_t v2, bool s);
    uint32_t logical_or(uint32_t v1, uint32_t v2, bool s);
    uint32_t logical_eor(uint32_t v1, uint32_t v2, bool s);
    void do_mul_cycles(uint32_t mul_operand);

    // interrupts
    //void request_interrupt(Interrupt interrupt);
    void do_interrupts();
    void service_interrupt();

    // timers
    void tick_timers(int cycles);
    void timer_overflow(int timer);

    // mode switching
    void switch_mode(cpu_mode new_mode);
    void store_registers(cpu_mode mode);
    void load_registers(cpu_mode mode);
    void set_cpsr(uint32_t v);
    cpu_mode cpu_mode_from_bits(uint32_t v);

    //flag helpers
    void set_negative_flag(uint32_t v);
    void set_zero_flag(uint32_t v);
    void set_nz_flag(uint32_t v);

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
    uint32_t regs[16] = {0};

    // flags
    // combined into cpsr when it is read
    bool flag_z;
    bool flag_n;
    bool flag_c;
    bool flag_v;



    // backup stores
    uint32_t user_regs[16] = {0};
    uint32_t pc_actual = 0;
    uint32_t cpsr = 0; // status reg

    // r8 - r12 banked
    uint32_t fiq_banked[5] = {0};

    // regs 13 and 14 banked
    uint32_t hi_banked[5][2] = {0};

    // banked status regs
    uint32_t status_banked[5] = {0};

    // in arm or thumb mode?
    bool is_thumb = false;

    bool dma_in_progress = false;

    // what context is the arm cpu in
    cpu_mode arm_mode;


    // cpu pipeline
    uint32_t pipeline[2] = {0};
};

}