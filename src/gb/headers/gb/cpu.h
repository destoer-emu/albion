#pragma once
#include "forward_def.h"
#include <destoer-emu/lib.h>

class Cpu
{
public:
    void init(Memory *m, Ppu *p,Apu *ap, Disass *d, Debug *debugger);
    void step();
    void cycle_tick(int cycles); 


    void request_interrupt(int interrupt);
    bool get_cgb() const {return is_cgb;}


    uint16_t internal_timer = 0;
    
    uint8_t joypad_state = 0xff;



    /* register reads for the debugger */

    uint16_t get_pc() const { return pc; }
    uint16_t get_sp() const { return sp; }
    uint16_t get_hl() const { return read_hl(); }
    uint16_t get_af() const { return read_af(); }
    uint16_t get_bc() const { return read_bc(); }
    uint16_t get_de() const { return read_de(); }

    // save states
    void save_state(std::ofstream &fp);
    void load_state(std::ifstream &fp);

private:
    Memory *mem;
    Ppu *ppu;
    Apu *apu;
    Disass *disass;
    Debug *debug;


    // registers
    uint8_t a; uint8_t f; //af
    uint8_t b; uint8_t c; //bc
    uint8_t d; uint8_t e; //de
    uint8_t h; uint8_t l; //hl
    uint16_t sp;
    uint16_t pc;


    // interrupts
    bool halt = false;
    bool ei = false;
    bool di = false;
    bool interrupt_enable = false;
    bool halt_bug = false;


    // timer
    void update_timers(int cycles);
    



    // cgb
    bool is_cgb = false;
    bool is_double = false;

    void write_af(uint16_t data);
    void write_bc(uint16_t data);
    void write_de(uint16_t data);
    void write_hl(uint16_t data);

    uint16_t read_af() const;
    uint16_t read_de() const;
    uint16_t read_bc() const;
    uint16_t read_hl() const;



    void exec_instr();
    void exec_cb(uint8_t cbop);
    void handle_instr_effects();


    // interrupts
    void service_interrupt(int interrupt);
    void do_interrupts();

    // instr helpers
    void set_zero(uint8_t reg);
    void instr_dec(uint8_t reg);
    void instr_inc(uint8_t reg);
    void instr_bit(uint8_t reg, uint8_t bit);
    void instr_and(uint8_t num);
    uint8_t instr_rl(uint8_t reg);
    void instr_sub(uint8_t num);
    void instr_cp(uint8_t num);
    void instr_sbc(uint8_t num);
    void instr_add(uint8_t num);
    uint16_t instr_addi(uint16_t reg, int8_t num);
    void instr_adc(uint8_t num);
    uint16_t instr_addw(uint16_t reg, uint16_t num);
    void instr_or(uint8_t val);
    uint8_t instr_swap(uint8_t num);
    void instr_xor(uint8_t num);
    uint8_t instr_sla(uint8_t reg);
    uint8_t instr_sra(uint8_t reg);
    uint8_t instr_srl(uint8_t reg);
    uint8_t instr_rr(uint8_t reg);
    uint8_t instr_rrc(uint8_t reg);
    uint8_t instr_rlc(uint8_t reg);
    void instr_jr();
    void instr_jr_cond(bool cond, int bit);
    void call_cond(bool cond, int bit);
    void ret_cond(bool cond, int bit);


    // stack helpers
    uint8_t read_stackt();
    void write_stackt(uint8_t v);
    uint16_t read_stackwt();
    void write_stackwt(uint16_t v);
    void write_stackw(uint16_t v);
    void write_stack(uint8_t v);

    void check_rst_loop(uint16_t addr, uint8_t op);

};


constexpr uint32_t Z = 7; // zero flag
constexpr uint32_t N = 6; // negative flag
constexpr uint32_t H = 5; // half carry flag
constexpr uint32_t C = 4; // carry flag

 