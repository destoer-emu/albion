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


    int get_vcount() const { return ly; } 

    void update_vcount_compare();

    void render_palette(uint32_t *palette, size_t size);

    
    uint32_t convert_color(uint16_t color)
    {
        return col_lut[deset_bit(color,15)];
    }

    std::vector<uint32_t> screen;
    bool new_vblank = false;
    DispIo disp_io;
    display_mode mode = display_mode::visible;
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
    
    bool window_0_y_triggered = false;
    bool window_1_y_triggered = false;

    Mem &mem;
    Cpu &cpu;

    
    std::vector<std::vector<TileData>> bg_lines;
    std::vector<TileData> bg_line;
    std::vector<TileData> sprite_line;
    std::vector<bool> sprite_semi_transparent;
    std::vector<window_source> window;
    std::vector<int> oam_priority;


    using ColorLut = std::array<uint32_t,32768>;
    constexpr ColorLut pop_color_lut()
    {
        ColorLut lut{};

        for(uint16_t c = 0; c < lut.size(); c++)
        {

            const uint32_t R = c & 0x1f;
            const uint32_t G = (c >> 5) & 0x1f;
            const uint32_t B = (c >> 10) & 0x1f;

            // default to standard colors until we add proper correction
            lut[c] =  B << 19 |  G << 11 | R << 3 | 0xFF000000;
        }

        return lut;
    }

    const ColorLut col_lut = pop_color_lut();
};

}