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

struct Display
{
    Display(GBA &gba);
    void init();
    void tick(int cycles);

    void update_vcount_compare();

    void insert_new_ppu_event();


    void render_palette(uint32_t *palette, size_t size);
    void render_map(int id, std::vector<uint32_t> &map);

    uint32_t convert_color(uint16_t color)
    {
        return col_lut[deset_bit(color,15)];
    }

    std::vector<uint32_t> screen;
    bool new_vblank = false;
    DispIo disp_io;
    display_mode mode = display_mode::visible;

    struct TileData
    {
        TileData() {}

        TileData(uint16_t c, pixel_source s)
        {
            color = c;
            source = s;
        }

        uint16_t color = 0;
        pixel_source source = pixel_source::bd;
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
    bool is_bg_window_trivial(int id);

    // renderer helper functions
    uint16_t read_bg_palette(uint32_t pal_num,uint32_t idx);
    uint16_t read_obj_palette(uint32_t pal_num,uint32_t idx);

    void read_tile(TileData *tile,unsigned int bg,bool col_256,uint32_t base,uint32_t pal_num,uint32_t tile_num, 
        uint32_t y,bool x_flip, bool y_flip);
    
    void draw_tile(uint32_t x,const TileData &p);

    unsigned int cyc_cnt = 0; // current number of elapsed cycles
    unsigned int ly = 0; // current number of cycles
    
    bool window_0_y_triggered = false;
    bool window_1_y_triggered = false;

    Mem &mem;
    Cpu &cpu;
    GBAScheduler &scheduler;

    struct Scanline
    {
        TileData t1;
        TileData t2;
    };

    std::vector<Scanline> scanline;


    std::vector<TileData> sprite_line;
    std::vector<bool> sprite_semi_transparent;
    std::vector<window_source> window;
    std::vector<uint32_t> oam_priority;
    std::vector<uint32_t> sprite_priority;


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
