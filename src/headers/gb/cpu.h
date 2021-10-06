#pragma once
#include <gb/forward_def.h>
#include <destoer-emu/lib.h>
#include <gb/debug.h>
#include <gb/scheduler.h>

namespace gameboy
{

// did the last instr have a side effect
// that happens after the instr has executed
enum class instr_state
{
    normal,
    halt,
    ei, // enable interrupt
    di  // disable interrupt
};


struct Cpu final
{
    Cpu(GB &gb);

    bool get_double() const;

    void insert_new_timer_event() noexcept;
    int get_next_timer_event() const noexcept;

    void update_intr_req() noexcept;
    void update_intr_fire() noexcept;


    // timer
    void update_timers(uint32_t cycles) noexcept;

    void init(bool use_bios = false);


#ifdef DEBUG
    EXEC_INSTR_FPTR exec_instr_fptr;

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


    void cycle_tick(uint32_t cycles) noexcept; 
    void cycle_tick_t(uint32_t cycles) noexcept;
    void tima_inc() noexcept;
    bool internal_tima_bit_set() const noexcept;
    bool tima_enabled() const noexcept;
    void tima_reload() noexcept;

    void request_interrupt(int interrupt) noexcept;

    // oam bug
    void oam_bug_write(uint16_t v);
    void oam_bug_read(uint16_t v);
    void oam_bug_read_increment(uint16_t v);


    uint16_t internal_timer = 0;
    
    uint8_t joypad_state = 0xff;

    // freq bits for internal timer
    static constexpr int freq_arr[4] = {9,3,5,7};


    // serial
    int serial_cyc;
    int serial_cnt;

    void tick_serial(int cycles) noexcept;
    void insert_new_serial_event() noexcept;

    uint16_t read_af() const noexcept
    {
        return (a << 8) | carry << C | half << H
            | zero << Z | negative << N;
    }



    void write_af(uint16_t v) noexcept 
    {
        a = (v & 0xff00) >> 8;
        carry = is_set(v,C);
        half = is_set(v,H);
        zero = is_set(v,Z);
        negative = is_set(v,N);
    }


    // honestly it would just be easier to use a union
    uint8_t read_lower(uint16_t v) const
    {
        uint8_t buf[2];
        memcpy(buf,&v,sizeof(buf));
        return buf[0];
    }

    uint8_t read_upper(uint16_t v) const
    {
        uint8_t buf[2];
        memcpy(buf,&v,sizeof(buf));
        return buf[1];
    }

    void write_lower(uint16_t *r, uint8_t v)
    {
        char *buf = reinterpret_cast<char*>(r);
        buf[0] = v;
    }

    void write_upper(uint16_t *r, uint8_t v)
    {
        char *buf = reinterpret_cast<char*>(r);
        buf[1] = v;
    }


    uint8_t read_h() const noexcept { return read_upper(hl); }
    uint8_t read_l() const noexcept { return read_lower(hl); }
    uint8_t read_a() const noexcept { return a; }
    uint8_t read_f() const noexcept 
    { 
        return carry << C | half << H
		| zero << Z | negative << N; 
    }
    uint8_t read_b() const noexcept { return read_upper(bc); }
    uint8_t read_c() const noexcept { return read_lower(bc); }
    uint8_t read_d() const noexcept { return read_upper(de); }
    uint8_t read_e() const noexcept { return read_lower(de); }

    bool read_flag_z() const noexcept { return zero; }
    bool read_flag_n() const noexcept { return negative; }
    bool read_flag_h() const noexcept { return half;}
    bool read_flag_c() const noexcept { return carry;}

    // save states
    void save_state(std::ofstream &fp);
    void load_state(std::ifstream &fp);

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

    Memory &mem;
    Apu &apu;
    Ppu &ppu;
    GameboyScheduler &scheduler;
    GBDebug &debug;
    Disass &disass;

    // registers
    uint8_t a; /*uint8_t f*/; //af
    uint16_t bc;
    uint16_t de;
    uint16_t hl;
    uint16_t sp;
    uint16_t pc;


    static constexpr uint32_t Z = 7; // zero flag
    static constexpr uint32_t N = 6; // negative flag
    static constexpr uint32_t H = 5; // half carry flag
    static constexpr uint32_t C = 4; // carry flag


    bool zero;
    bool negative;
    bool half;
    bool carry;


    // interrupts
    instr_state instr_side_effect = instr_state::normal;
    bool interrupt_enable = false;
    bool interrupt_req = false;
    bool interrupt_fire = false;
    bool halt_bug = false;

    // next opcode
    uint8_t opcode = 0;

    // cgb
    bool is_cgb = false;
    bool is_double = false;

    bool is_sgb = false;


    void handle_halt();

    void switch_double_speed() noexcept;

    uint8_t fetch_opcode() noexcept;

    // interrupts
    void do_interrupts() noexcept;

    // instr helpers

    template<const int REG>
    void write_r16_group1(uint16_t v);

    template<const int REG>
    void write_r8(uint8_t v);

    template<const int REG>
    void write_r16_group3(uint16_t v);

    template<const int REG>
    void write_r16_group2(uint16_t v);

    template<const int REG>
    uint16_t read_r16_group1();

    template<const int REG>
    uint16_t read_r16_group3();

    template<const int REG>
    uint16_t read_r16_group2();

    template<const int REG>
    uint8_t read_r8();


    template<const int COND>
    bool cond();

    void undefined_opcode();
    void undefined_opcode_cb();
    void ld_u16_sp();
    void nop();
    void jp();
    void di();

    template<const int REG>
    void ld_r16_u16();

    void ld_u16_a();

    template<const int REG>
    void ld_r8_u8();

    void ld_ffu8_a();
    void call();
    void halt();

    template<const int DST, const int SRC>
    void ld_r8_r8();

    void jr();
    void ret();

    template<const int REG>
    void push();

    template<const int REG>
    void pop();

    template<const int REG>
    void inc_r16();


    template<const int REG>
    void ld_a_r16();

    void set_zero(uint8_t v);

    void instr_or(uint8_t v);

    template<const int REG>
    void or_r8();

    template<const int COND>
    void jr_cond();

    void ld_a_ffu8();

    void instr_cp(uint8_t v);

    template<const int REG>
    void cp_r8();

    void cp_u8();
    void or_u8();
    void ld_a_u16();
    void instr_and(uint8_t v);
    void and_u8();

    template<const int REG>
    void and_r8();

    template<const int COND>
    void call_cond();

    template<const int REG>
    void dec_r8();

    template<const int REG>
    void inc_r8();

    void instr_xor(uint8_t v);

    template<const int REG>
    void xor_r8();

    void xor_u8();

    template<const int REG>
    void ld_r16_a();

    void instr_add(uint8_t v);

    template<const int REG>
    void add_r8();

    void add_u8();


    void instr_sub(uint8_t v);

    template<const int REG>
    void sub_r8();

    void sub_u8();


    void instr_adc(uint8_t v);

    template<const int REG>
    void adc_r8();

    void adc_u8();

    template<const int COND>
    void ret_cond();

    template<const int REG>
    void add_hl_r16();

    void jp_hl();

    template<const int COND>
    void jp_cond();

    uint16_t instr_addi(int8_t v);
    void ld_hl_sp_i8();
    void daa();

    void ld_sp_hl();

    void ei();

    void stop();

    template<const int REG>
    void dec_r16();

    void add_sp_i8();


    void instr_sbc(uint8_t v);

    template<const int REG>
    void sbc_r8();

    void sbc_u8();

    void reti();

    template<const int ADDR, const int OP>
    void rst();

    void ld_a_ff00_c();
    void ld_ff00_c_a();

    void cpl();
    void scf();
    void ccf();

    // cb ops

    void cb_opcode();

    template<const int REG>
    void srl();

    uint8_t instr_rrc(uint8_t v);

    template<const int REG>
    void rrc_r8();

    void rrca();

    template<const int REG>
    void rr_r8();
    
    uint8_t instr_rr(uint8_t v);
    void rra();

    template<const int REG>
    void instr_swap();

    uint8_t instr_rlc(uint8_t v);
    void rlca();

    template<const int REG>
    void rlc_r8();


    uint8_t instr_rl(uint8_t v);

    template<const int REG>
    void rl_r8();

    void rla();


    template<const int REG>
    void sla_r8();


    template<const int REG>
    void sra_r8();

    template<const int REG, const int BIT>
    void bit_r8();

    template<const int REG, const int BIT>
    void res_r8();

    template<const int REG, const int BIT>
    void set_r8();

    // stack helpers
    uint8_t read_stackt() noexcept;
    void write_stackt(uint8_t v) noexcept;
    uint16_t read_stackwt() noexcept;
    void write_stackwt(uint16_t v) noexcept;
    void write_stackw(uint16_t v) noexcept;
    void write_stack(uint8_t v) noexcept;


    // oam bug
    uint32_t get_cur_oam_row() const;
    bool oam_should_corrupt(uint16_t v) const noexcept;
};
}