#pragma once
#include <destoer-emu/lib.h>
namespace gameboyadvance
{
// reference point registers
// bg2x and bg2y
// bg3x and bg3y
struct RefPoint 
{
    RefPoint();
    void init();

    // write only
    void write(int idx, uint8_t v);

    // bg reference point registers
    // 28bit signed write only
    uint32_t ref_point;
};


struct BgCnt
{
    BgCnt();
    void init();

    void write(int idx, uint8_t v);
    uint8_t read(int idx) const;


    int priority;
    int char_base_block;
    bool mosaic;
    bool col_256;
        

    int screen_base_block;
    int area_overflow;
    int screen_size;
};


struct BgOffset
{
    BgOffset();
    void init();

    // write only
    void write(int idx, uint8_t v);

    uint16_t offset;
};

struct DispCnt
{
    DispCnt();
    void init();

    uint8_t read(int idx) const;
    void write(int idx, uint8_t v);

    int bg_mode;
    int display_frame;
    bool hblank_free;
    bool obj_vram_mapping;
    bool forced_blank;
    bool bg_enable[4];
    bool obj_enable;
    bool window0_enable;
    bool window1_enable;
    bool obj_window_enable;
};


struct DispStat
{
    DispStat();
    void init();

    uint8_t read(int idx) const;
    void write(int idx, uint8_t v);


    bool vblank;
    bool hblank;
    bool lyc_hit;
    bool vblank_irq_enable;
    bool hblank_irq_enable;
    bool lyc_irq_enable;
    int lyc;
};

struct DispIo
{
    DispIo();
    void init();





    RefPoint bg2x;
    RefPoint bg2y; 

    std::array<BgCnt,4> bg_cnt;
    std::array<BgOffset,4> bg_offset_x;
    std::array<BgOffset,4> bg_offset_y;

    DispCnt disp_cnt;
    DispStat disp_stat;
};

}
