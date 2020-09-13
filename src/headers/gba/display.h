#pragma once
#include <destoer-emu/lib.h>
#include <gba/forward_def.h>
#include <gba/disp_io.h>

namespace gameboyadvance
{
static constexpr uint32_t SCREEN_WIDTH = 240;
static constexpr uint32_t SCREEN_HEIGHT = 160;

enum class display_mode
{
    visible,hblank,vblank
};

class Display
{
public:
    Display(GBA &gba);
    void init();
    void tick(int cycles);

    display_mode get_mode() const { return mode; }
    void set_mode(display_mode mode) { this->mode = mode; }
    void set_cycles(int cycles) { cyc_cnt = cycles; }
    int get_vcount() const { return ly; } 

    void update_vcount_compare();

    void render_palette(uint32_t *palette, size_t size);

    std::vector<uint32_t> screen;
    bool new_vblank = false;
    DispIo disp_io;
private:

    struct TileData
    {
        TileData() {}

        TileData(uint16_t c, uint32_t p, uint32_t b)
        {
            col_num = c;
            pal_num = p;
            bg = b;
        }

        uint16_t col_num = 0;
        uint32_t pal_num = 0;
        uint32_t bg = 0;
    };

    void render();
    void render_text(int id);
    void render_affine(int id);
    void advance_line();
    void render_sprites(int mode);
    void merge_layers();

    // is this inside a window if so is it enabled?
    bool bg_window_enabled(unsigned int bg, unsigned int x) const;

    // are sprites enabled inside a window
    bool sprite_window_enabled(unsigned int x) const;

    // are special effects enabled inside a window
    bool special_window_enabled(unsigned int x) const;

    void cache_window();

    // renderer helper functions
    uint16_t read_bg_palette(uint32_t pal_num,uint32_t idx);
    uint16_t read_obj_palette(uint32_t pal_num,uint32_t idx);
    uint16_t get_color(const TileData &data, const pixel_source source);
    void read_tile(TileData tile[],unsigned int bg,bool col_256,uint32_t base,uint32_t pal_num,uint32_t tile_num, 
        uint32_t y,bool x_flip, bool y_flip);


    unsigned int cyc_cnt = 0; // current number of elapsed cycles
    unsigned int ly = 0; // current number of cycles
    
    Mem &mem;
    Cpu &cpu;

    display_mode mode = display_mode::visible;

    std::vector<std::vector<TileData>> bg_lines;
    std::vector<TileData> bg_line;
    std::vector<TileData> sprite_line;
    std::vector<bool> sprite_semi_transparent;
    std::vector<window_source> window;
    std::vector<int> oam_priority;
};

// this needs color correction at some point
inline uint32_t convert_color(uint16_t color)
{
    int r = color & 0x1f;
    int g = (color >> 5) & 0x1f;
    int b = (color >> 10) & 0x1f;



    return b << 19 |  g << 11 | r << 3 | 0xFF000000;
}

}