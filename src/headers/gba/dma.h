#pragma once
#include <gba/forward_def.h>
#include <destoer-emu/lib.h>

namespace gameboyadvance
{

enum class dma_type
{
    immediate = 0 ,vblank = 1,
    hblank = 2, special = 3 
};


class Dma
{
public: 
    void init(Mem *mem, Cpu *cpu);

    void handle_dma(dma_type req_type, int special_dma = -1);

    void write_src(int reg_num, int idx, uint8_t v);
    void write_dst(int reg_num, int idx, uint8_t v);
    void write_count(int reg_num, int idx, uint8_t v);
    void write_control(int reg_num, int idx, uint8_t v);
    uint8_t read_cnt(int reg_num, int idx) const;

private:
    Mem *mem = nullptr;
    Cpu *cpu = nullptr;

    void do_dma(int reg_num,dma_type req_type);
    static constexpr int max_count[4] = {0x4000,0x4000,0x4000,0x10000};

    bool dma_in_progress;

    // only the control register is readable
    uint32_t src[4];
    uint32_t dst[4];

    uint16_t count[4];
    uint16_t count_shadow[4];

    struct DmaCnt
    {
        int dst_cnt; // 0 = inc, 1 = dec, 3 = inc/reload
        int src_cnt; // 0 = inc, 1 = dec, 3 = invalid
        bool dma_repeat;
        bool is_word; // else 16 bit
        bool drq; // game pak 3 only
        dma_type start_time; // immediatly, vblank, hblank, special
        bool irq; // irq at end of word count
        bool enable;
    };

    DmaCnt control[4];
};

};