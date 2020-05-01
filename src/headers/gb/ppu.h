#pragma once
#include "forward_def.h"
#include <destoer-emu/lib.h>


namespace gameboy
{

static constexpr uint32_t SCREEN_WIDTH = 160;
static constexpr uint32_t SCREEN_HEIGHT = 144;

enum class ppu_mode
{
    oam_search = 2,
    pixel_transfer = 3,
    hblank = 0,
    vblank = 1
};


enum class dmg_colors
{
    white = 1,
    light_gray =  2,
    dark_gray =  3,
    black =  4
};


struct Obj // struct for holding sprites on a scanline
{
    uint16_t index = 0;
    uint8_t x_pos = 0;
};

class Ppu
{
public:
    Ppu(GB &gb);


    void init() noexcept;

    std::vector<uint32_t> screen; // 160 by 144



    ppu_mode mode = ppu_mode::oam_search;

    int current_line = 0;
    bool new_vblank = false;

    void update_graphics(int cycles) noexcept;

    void set_scanline_counter(int v) noexcept
    {
        scanline_counter = v;
    }


    void write_stat() noexcept;

    void turn_lcd_off() noexcept;
    void turn_lcd_on() noexcept; 
    void window_disable() noexcept;

    // mode change, write, or line change
    // will trigger this
    void stat_update() noexcept;


    // cgb 
    void set_bg_pal_idx(uint8_t v) noexcept;
    void set_sp_pal_idx(uint8_t v) noexcept;
    void write_sppd(uint8_t v) noexcept;
    void write_bgpd(uint8_t v) noexcept;


    uint8_t get_sppd() const noexcept;
    uint8_t get_bgpd() const noexcept;


    // save states
    void save_state(std::ofstream &fp);
    void load_state(std::ifstream &fp);


    // display viewer
    std::vector<uint32_t> render_bg(bool higher) noexcept;
    std::vector<uint32_t> render_tiles() noexcept;
    void render_palette(uint32_t *palette_bg,uint32_t *palette_sp) noexcept;

private:

    Cpu &cpu;
    Memory &mem;    

    enum class pixel_source
    {
        tile = 0,
        tile_cgbd = 3, // priority over everything when set in tile attr
        sprite_zero = 1,
        sprite_one = 2
    };


    // pod type to hold pixels in the fifo
    // cgb_pal is part of attr not sure it should 
    // just be anded out each time
    struct Pixel_Obj 
    {
        int colour_num = 0;
        pixel_source source = pixel_source::tile;
        int cgb_pal = 0;
        bool scx_a = false;   
        uint8_t attr = 0;   
        int priority = 0;
    };

    template<size_t N>
    struct Pixel_Fifo
    {
        void reset() noexcept
        {
            len = 0;
            read_idx = 0;
        }

        Pixel_Obj fifo[N];
        const size_t size = N;
        int len = 0;
        int read_idx = 0;
    };



    bool sprite_win(const Pixel_Obj &sp, const Pixel_Obj &bg) noexcept;
    bool push_pixel() noexcept;
    void tick_fetcher(int cycles) noexcept;
    void draw_scanline(int cycles) noexcept;
    void tile_fetch() noexcept;
    dmg_colors get_colour(uint8_t colour_num, uint16_t address) noexcept;
    uint32_t get_cgb_color(int color_num, int cgb_pal, pixel_source source) noexcept;
    uint32_t get_dmg_color(int color_num, pixel_source source) noexcept;
    void read_sprites() noexcept;
    bool sprite_fetch() noexcept;  


    void reset_fetcher() noexcept;

    // main ppu state
	bool signal = false;
    uint32_t scanline_counter = 0;


	// fetcher
	bool hblank = false;
	int x_cord = 0; // current x cord of the ppu
	Pixel_Obj ppu_fifo[168];
	int pixel_idx = 0;

	uint8_t ppu_cyc = 0; // how far for a tile fetch is
	uint8_t ppu_scyc = 0; // how far along a sprite fetch is
	int pixel_count = 0; // how many pixels are in the fifo
	Pixel_Obj fetcher_tile[8];

    // sprites with the higher priority will stay in this queue
    // that are non zero color (i.e appear eailer in objects_priority)
    Pixel_Fifo<8> fetcher_sprite;

	int tile_cord = 0;
	bool tile_ready = false; // is the tile fetch ready to go into the fio 
	Obj objects_priority[10]; // sprites for the current scanline
	int no_sprites = 0; // how many sprites
    int cur_sprite = 0; // current sprite
	bool sprite_drawn = false;
	bool window_start = false;
	bool x_scroll_tick = false;
	int scx_cnt = 0;


    // window internal vars
    // keep track of how much the window has drawn
    int window_y_line = 0;
    int window_x_line = 0;
    bool window_drawn = false; // did we draw the window on this line?



    // cgb pal
	uint8_t bg_pal[0x40] = {0xff}; // bg palette data
	uint8_t sp_pal[0x40] = {0xff}; // sprite pallete data 
	int sp_pal_idx = 0;
	int bg_pal_idx = 0; // index into the bg pal (entry takes two bytes)


};

}