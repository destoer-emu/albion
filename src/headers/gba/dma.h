#pragma once
#include <gba/forward_def.h>
#include <destoer-emu/lib.h>

namespace gameboyadvance
{

enum class dma_type
{
    immediate,
    vblank,
    hblank,
    fifo_a,
    fifo_b,
    video_capture,
    invalid
};


struct DmaReg
{
    DmaReg(int reg);
    void init();

    uint32_t src;
    uint32_t dst;
    uint16_t word_count;
    
    // internal copies reloaded upon enable
    // or upon repeat
    uint32_t src_shadow;
    uint32_t dst_shadow;
    uint16_t word_count_shadow;
    uint32_t word_offset;


    int dst_cnt; // 0 = inc, 1 = dec, 3 = inc/reload
    int src_cnt; // 0 = inc, 1 = dec, 3 = invalid
    bool dma_repeat;
    bool is_word; // else 16 bit
    bool drq; // game pak 3 only
    int transfer_type;  // immediatly, vblank, hblank, special
    dma_type start_time;
    bool irq; // irq at end of word count
    bool enable;
    static constexpr int max_counts[4] = {0x4000,0x4000,0x4000,0x10000};
    const int max_count;


    static constexpr interrupt dma_interrupts[4] = {interrupt::dma0,interrupt::dma1,interrupt::dma2,interrupt::dma3}; 
    const interrupt dma_interrupt;

    bool interrupted;
};



class Dma
{
public: 
    Dma(GBA &gba);
    void init();

    void handle_dma(dma_type req_type);

    void write_source(int reg_num,int idx, uint8_t v);
    void write_dest(int reg_num,int idx, uint8_t v);
    void write_count(int reg_num,int idx, uint8_t v);

    void write_control(int reg_num,int idx, uint8_t v);
    uint8_t read_control(int reg_num,int idx);

    void turn_off_video_capture();

    int active_dma;
    std::array<DmaReg,4> dma_regs{0,1,2,3};

private:
    Mem &mem;
    Cpu &cpu;
    Debug &debug;

    void do_dma(int reg_num,dma_type req_type);
    void check_dma(dma_type req_type);
    void handle_increment(int reg_num);

    static constexpr int32_t addr_increment_table[2][4] = 
    {
        {+ARM_HALF_SIZE,-ARM_HALF_SIZE,0,+ARM_HALF_SIZE}, // half
        {+ARM_WORD_SIZE,-ARM_WORD_SIZE,0,+ARM_WORD_SIZE} // word
    };

    std::vector<dma_type> req_list;
};

};