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
    sound,
    video_capture
};


struct DmaReg
{
    DmaReg();
    void init();

    uint32_t src;
    uint32_t dst;
    uint16_t word_count;
    
    // internal copies reloaded upon enable
    // or upon repeat
    uint32_t src_shadow;
    uint32_t dst_shadow;
    uint16_t word_count_shadow;


    int dst_cnt; // 0 = inc, 1 = dec, 3 = inc/reload
    int src_cnt; // 0 = inc, 1 = dec, 3 = invalid
    bool dma_repeat;
    bool is_word; // else 16 bit
    bool drq; // game pak 3 only
    int transfer_type;  // immediatly, vblank, hblank, special
    dma_type start_time;
    bool irq; // irq at end of word count
    bool enable;
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


private:
    Mem &mem;
    Cpu &cpu;
    Debug &debug;

    void do_dma(int reg_num,dma_type req_type);
    void handle_increment(int reg_num);
    static constexpr int max_count[4] = {0x4000,0x4000,0x4000,0x10000};

    bool dma_in_progress;

    std::array<DmaReg,4> dma_regs;
};

};