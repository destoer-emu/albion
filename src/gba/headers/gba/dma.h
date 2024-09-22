#pragma once
#include <gba/forward_def.h>
#include <albion/lib.h>

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

    u32 src = 0;
    u32 dst = 0;
    u16 word_count = 0;
    
    // internal copies reloaded upon enable
    // or upon repeat
    u32 src_shadow = 0;
    u32 dst_shadow = 0;
    u16 word_count_shadow = 0;


    int dst_cnt = 0; // 0 = inc, 1 = dec, 3 = inc/reload
    int src_cnt = 0; // 0 = inc, 1 = dec, 3 = invalid
    bool dma_repeat = false;
    bool is_word = false; // else 16 bit
    bool drq = false; // game pak 3 only
    int transfer_type = 0;  // immediatly, vblank, hblank, special
    dma_type start_time = dma_type::immediate;
    bool irq = false; // irq at end of word count
    bool enable = false;
    static constexpr int max_counts[4] = {0x4000,0x4000,0x4000,0x10000};
    const int max_count = false;


    static constexpr interrupt dma_interrupts[4] = {interrupt::dma0,interrupt::dma1,interrupt::dma2,interrupt::dma3}; 
    const interrupt dma_interrupt;

    bool interrupted = false;
};



struct Dma
{
    Dma(GBA &gba);
    void init();

    void handle_dma(dma_type req_type);

    void write_source(int reg_num,int idx, u8 v);
    void write_dest(int reg_num,int idx, u8 v);
    void write_count(int reg_num,int idx, u8 v);

    void write_control(int reg_num,int idx, u8 v);
    u8 read_control(int reg_num,int idx);

    void turn_off_video_capture();

    int active_dma = -1;
    std::array<DmaReg,4> dma_regs{0,1,2,3};


    Mem &mem;
    Cpu &cpu;
    GBAScheduler &scheduler;
    Debug &debug;

    void do_dma(int reg_num,dma_type req_type);
    void handle_increment(int reg_num);
    bool do_fast_dma(int reg_num);
    void check_dma();


    static constexpr int32_t addr_increment_table[2][4] = 
    {
        {+ARM_HALF_SIZE,-ARM_HALF_SIZE,0,+ARM_HALF_SIZE}, // half
        {+ARM_WORD_SIZE,-ARM_WORD_SIZE,0,+ARM_WORD_SIZE} // word
    };

    std::array<bool,4> dma_request;
    u32 req_count = 0;
};

};