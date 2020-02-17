#pragma once
#include "forward_def.h"
#include <destoer-emu/lib.h>


enum display_mode
{
    visible,hblank,vblank
};

class Display
{
public:
    void init(Mem *mem, Cpu *cpu);
    void tick(int cycles);

    display_mode get_mode() const { return mode; }
    void set_mode(display_mode mode) { this->mode = mode; }
    void set_cycles(int cycles) { cyc_cnt = cycles; }
    void load_reference_point_regs();

    static constexpr int X = 240;
    static constexpr int Y = 160;    
    std::vector<uint32_t> screen;
    bool new_vblank = false;
private:

    uint32_t reference_point_x;
    uint32_t reference_point_y;

    void render();
    void render_text(int id);
    void advance_line();



    // renderer helper functions
    uint16_t read_palette(uint32_t pal_num,uint32_t idx);
    void read_tile(uint32_t tile[],bool col_256,uint32_t base,uint32_t pal_num,uint32_t tile_num, 
        uint32_t y,bool x_flip, bool y_flip);


    int cyc_cnt = 0; // current number of elapsed cycles
    int ly = 0; // current number of cycles
    
    Mem *mem = nullptr;
    Cpu *cpu = nullptr;

    display_mode mode = display_mode::visible;
};


inline uint32_t convert_color(uint16_t color)
{
    int r = color & 0x1f;
    int g = (color >> 5) & 0x1f;
    int b = (color >> 10) & 0x1f;



    return b << 19 |  g << 11 | r << 3 | 0xFF000000;
}
