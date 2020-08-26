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
    void write_x(int idx, uint8_t v);
    void write_y(int idx, uint8_t v);

    // bg reference point registers
    // 28bit signed write only
    int32_t ref_point_x;
    int32_t ref_point_y;

    // internal reference points
    int32_t int_ref_point_x;
    int32_t int_ref_point_y;

private:
    void write(int idx, uint8_t v, int32_t &ref_point, int32_t &int_ref_point);

};


struct BgCnt
{
    BgCnt();
    void init();

    void write(int idx, uint8_t v);
    uint8_t read(int idx) const;


    unsigned int priority;
    unsigned int char_base_block;
    bool mosaic;
    bool col_256;
        

    unsigned int screen_base_block;
    unsigned int area_overflow;
    unsigned int screen_size;
};


struct Window
{

    Window();
    void init();

    uint8_t read(int idx) const;
    void write(int idx, uint8_t v);



    bool bg_enable_lower[4];
    bool obj_enable_lower;
    bool special_enable_lower;


    bool bg_enable_upper[4];
    bool obj_enable_upper;
    bool special_enable_upper;
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
    unsigned int lyc;
};

// eg bg2pa
// theres likely a nicer way to do this one
// thatwill still make it easy to use in the display class
struct ScalingParam
{
    ScalingParam();
    void init();

    // write only
    void write_a(int idx,uint8_t v);
    void write_b(int idx,uint8_t v);
    void write_c(int idx,uint8_t v);
    void write_d(int idx,uint8_t v);

    int16_t a; //dx
    int16_t b; //dmx
    int16_t c; //dy
    int16_t d; //dmy

private:
    void write(int idx, uint8_t v, int16_t &param);
};

struct WindowDimensionH
{
    WindowDimensionH();
    void init();

    void write(int idx, uint8_t v);

    uint8_t x2; // rightmost + 1
    uint8_t x1; // leftmost
};


struct WindowDimensionV
{
    WindowDimensionV();
    void init();


    void write(int idx, uint8_t v);

    uint8_t y2; // bottommost + 1
    uint8_t y1; // leftmost
};


struct DispIo
{
    DispIo();
    void init();


    ScalingParam bg2_scale_param; 
    ScalingParam bg3_scale_param; 

    RefPoint bg2_ref_point;
    RefPoint bg3_ref_point;

    WindowDimensionH win0h;
    WindowDimensionH win1h;

    WindowDimensionV win0v;
    WindowDimensionV win1v;

    Window win_in;
    Window win_out;

    std::array<BgCnt,4> bg_cnt;
    std::array<BgOffset,4> bg_offset_x;
    std::array<BgOffset,4> bg_offset_y;

    DispCnt disp_cnt;
    DispStat disp_stat;
};
}
