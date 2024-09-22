#pragma once
#include "forward_def.h"
#include <albion/lib.h>
#include <gb/scheduler.h>

namespace gameboy
{

static constexpr u32 SCREEN_WIDTH = 160;
static constexpr u32 SCREEN_HEIGHT = 144;

enum class ppu_mode
{
    oam_search = 2,
    pixel_transfer = 3,
    hblank = 0,
    vblank = 1
};


struct Obj // struct for holding sprites on a scanline
{
    u16 index = 0;
    u8 x_pos = 0;
};

struct Ppu
{
    Ppu(GB &gb);


    void init() noexcept;
    ppu_mode get_mode() const noexcept;

    int get_next_ppu_event() const noexcept;
    void insert_new_ppu_event() noexcept;

    std::vector<u32> rendered; 
    std::vector<u32> screen; // 160 by 144

    // inform ppu that registers that can affect
    // pixel transfer have been written
    void ppu_write() noexcept;
    
    bool glitched_oam_mode = false;

    bool new_vblank = false;

    void update_graphics(u32 cycles) noexcept;

    unsigned int get_current_line() const noexcept
    {
        return early_line_zero? 0 : current_line;
    }

    u8 read_stat() const noexcept;


    void write_stat() noexcept;

    void turn_lcd_off() noexcept;
    void turn_lcd_on() noexcept; 
    void window_disable() noexcept;

    // mode change, write, or line change
    // will trigger this
    void stat_update() noexcept;


    // cgb 
    void set_bg_pal_idx(u8 v) noexcept;
    void set_sp_pal_idx(u8 v) noexcept;
    void write_sppd(u8 v) noexcept;
    void write_bgpd(u8 v) noexcept;


    u8 get_sppd() const noexcept;
    u8 get_bgpd() const noexcept;


    // save states
    void save_state(std::ofstream &fp);
    void load_state(std::ifstream &fp);


    // display viewer
    std::vector<u32> render_bg(bool higher) noexcept;
    std::vector<u32> render_tiles() noexcept;
    void render_palette(u32 *palette_bg,u32 *palette_sp) noexcept;


    // todo properly handle cgb in dmg
	u32 dmg_pal[3][4] = 
	{
		{0xffffffff,0xffcccccc,0xff777777,0xff000000},
		{0xffffffff,0xffcccccc,0xff777777,0xff000000},
		{0xffffffff,0xffcccccc,0xff777777,0xff000000}
	};

    using ColorLut = std::array<u32,32768>;
    const ColorLut col_lut = pop_color_lut();

    enum class mask_mode
    {
        cancel = 0,
        freeze = 1,
        black = 2,
        clear = 3
    };

    mask_mode mask_en;

    Cpu &cpu;
    Memory &mem;
    GameboyScheduler &scheduler;    

    enum class pixel_source
    {
        tile = 0,
        tile_cgbd = -1, // priority over everything when set in tile attr
        sprite_zero = 1,
        sprite_one = 2
    };

    struct Obj // struct for holding sprites on a scanline
    {
        u16 index = 0;
        u8 x_pos = 0;
        u8 attr = 0;
        unsigned priority = 0;
    };


    // pod type to hold pixels in the fifo
    // cgb_pal is part of attr not sure it should 
    // just be anded out each time
    struct Pixel_Obj 
    {
        size_t colour_num = 0;
        pixel_source source = pixel_source::tile;
        size_t cgb_pal = 0;
        size_t sprite_idx = 0;
    };


    struct Pixel_Fifo
    {
        void reset() noexcept
        {
            read_idx = 0;
            write_idx = 0;
            len = 0;
        }


        static constexpr size_t size = 16;
        Pixel_Obj fifo[size] = {};

        // implemented as a circular buffer
        size_t read_idx = 0;
        size_t write_idx = 0;
        size_t len = 0;
    };


    enum class fetcher_mode
    {
        tile,
        sprite
    };

    struct Pixel_Fetcher
    {
        void reset() noexcept
        {
            cyc = 0;
            mode = fetcher_mode::tile;
            ready = false;
            len = 0;
        }

        unsigned int cyc = 0; // how far for a tile fetch is
        fetcher_mode mode = fetcher_mode::tile;
        bool ready = false;
        Pixel_Obj buf[8];
        unsigned int len = 0;
    };




    bool sprite_win(const Pixel_Obj &sp, const Pixel_Obj &bg) noexcept;
    bool push_pixel() noexcept;
    void tick_fetcher() noexcept;
    void render_scanline() noexcept;
    void tile_fetch(Pixel_Obj *buf, bool use_window) noexcept;
    u32 get_cgb_color(int color_num, int cgb_pal, pixel_source source) const noexcept;
    u32 get_dmg_color(int color_num, pixel_source source) const noexcept;
    void read_sprites() noexcept;
    void sprite_fetch(Pixel_Obj *buf,bool use_fifo=true) noexcept;
    void switch_hblank() noexcept;

    bool window_active() const noexcept;


    void reset_fetcher() noexcept;


    // scanline drawing (used when no pixel transfer writes happen)
    void draw_scanline(u32 cycles) noexcept;


    // main ppu state
    ppu_mode mode = ppu_mode::oam_search;
	bool signal = false;
    u32 scanline_counter = 0;
    unsigned int current_line = 0;

    static constexpr u32 OAM_END = 80;
    static constexpr u32 LINE_END = 456;
    u32 pixel_transfer_end = 252;
    bool emulate_pixel_fifo = false;


    u32 calc_pixel_transfer_end() noexcept;

    Pixel_Fetcher fetcher;
    Pixel_Fifo bg_fifo;
    Pixel_Fifo obj_fifo;

	// enough to allow two tiles either side
	// plus the odd window and scx cords
	// 160 + 8 + 8 + 8 + 8
	Pixel_Obj scanline_fifo[194];

	Obj objects[10]; // sprites for the current scanline
	unsigned int no_sprites = 0; // how many sprites
    unsigned int cur_sprite = 0; // current sprite


    bool early_line_zero = false;

	unsigned int x_cord = 0; // current x cord of the ppu
	unsigned int tile_cord = 0;
	int scx_cnt = 0;


    // window internal vars
    // keep track of how much the window has drawn
    unsigned int window_y_line = 0;
    unsigned int window_x_line = 0;
    bool window_x_triggered = false;
    bool window_y_triggered = false;

    // cgb pal
	u8 bg_pal[0x40] = {0xff}; // bg palette data
	u8 sp_pal[0x40] = {0xff}; // sprite pallete data 
	unsigned int sp_pal_idx = 0;
	unsigned int bg_pal_idx = 0; // index into the bg pal (entry takes two bytes)


    // default colors
    static constexpr u32 dmg_colors[3][4] = 
    {
        {0xffffffff,0xffcccccc,0xff777777,0xff000000},
        {0xffffffff,0xffcccccc,0xff777777,0xff000000},
        {0xffffffff,0xffcccccc,0xff777777,0xff000000}
    };		


    constexpr ColorLut pop_color_lut()
    {
        ColorLut lut{};

        for(u16 col = 0; col < lut.size(); col++)
        {
            int blue = col & 0x1f;
            int green = (col >> 5) & 0x1f;
            int red = (col >> 10) & 0x1f;
            
            // convert rgb15 to rgb888
            red = (red << 3) | (red >> 2);
            blue = (blue << 3) | (blue >> 2);
            green = (green << 3) | (green >> 2);


            const u32 full_color = blue | (green << 8) | (red << 16);

            lut[col] = full_color | 0xff000000;
        }

        return lut;
    }

};

}