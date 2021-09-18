#pragma once
#include "forward_def.h"
#include <destoer-emu/lib.h>
#include <gb/scheduler.h>

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
    ppu_mode get_mode() const noexcept;

    int get_next_ppu_event() const noexcept;
    void insert_new_ppu_event() noexcept;

    bool using_fifo() const noexcept
    { 
        return emulate_pixel_fifo; 
    }


    std::vector<uint32_t> screen; // 160 by 144

    // inform ppu that registers that can affect
    // pixel transfer have been written
    void ppu_write() noexcept;
    
    bool glitched_oam_mode = false;

    bool new_vblank = false;

    void update_graphics(uint32_t cycles) noexcept;

    unsigned int get_current_line() const noexcept
    {
        return early_line_zero? 0 : current_line;
    }

    uint8_t read_stat() const noexcept;


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


    // todo properly handle cgb in dmg
	u32 dmg_pal[3][4] = 
	{
		{0xffffffff,0xffcccccc,0xff777777,0xff000000},
		{0xffffffff,0xffcccccc,0xff777777,0xff000000},
		{0xffffffff,0xffcccccc,0xff777777,0xff000000}
	};

    using ColorLut = std::array<uint32_t,32768>;
    const ColorLut col_lut = pop_color_lut();

    enum class mask_mode
    {
        cancel = 0,
        freeze = 1,
        black = 2,
        clear = 3
    };

    mask_mode mask_en;
private:

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
        uint16_t index = 0;
        uint8_t x_pos = 0;
        uint8_t attr = 0;
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
    uint32_t get_cgb_color(int color_num, int cgb_pal, pixel_source source) const noexcept;
    uint32_t get_dmg_color(int color_num, pixel_source source) const noexcept;
    void read_sprites() noexcept;
    void sprite_fetch(Pixel_Obj *buf,bool use_fifo=true) noexcept;
    void switch_hblank() noexcept;

    bool window_active() const noexcept;


    void reset_fetcher() noexcept;


    // scanline drawing (used when no pixel transfer writes happen)
    void draw_scanline(uint32_t cycles) noexcept;


    // main ppu state
    ppu_mode mode = ppu_mode::oam_search;
	bool signal = false;
    uint32_t scanline_counter = 0;
    unsigned int current_line = 0;

    static constexpr uint32_t OAM_END = 80;
    static constexpr uint32_t LINE_END = 456;
    uint32_t pixel_transfer_end = 252;
    bool emulate_pixel_fifo = false;


    uint32_t calc_pixel_transfer_end() noexcept;

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
	uint8_t bg_pal[0x40] = {0xff}; // bg palette data
	uint8_t sp_pal[0x40] = {0xff}; // sprite pallete data 
	unsigned int sp_pal_idx = 0;
	unsigned int bg_pal_idx = 0; // index into the bg pal (entry takes two bytes)


    // default colors
    static constexpr uint32_t dmg_colors[3][4] = 
    {
        {0xffffffff,0xffcccccc,0xff777777,0xff000000},
        {0xffffffff,0xffcccccc,0xff777777,0xff000000},
        {0xffffffff,0xffcccccc,0xff777777,0xff000000}
    };		


    constexpr ColorLut pop_color_lut()
    {
        ColorLut lut{};

        for(uint16_t col = 0; col < lut.size(); col++)
        {
            int blue = col & 0x1f;
            int green = (col >> 5) & 0x1f;
            int red = (col >> 10) & 0x1f;
            
            // convert rgb15 to rgb888
            red = (red << 3) | (red >> 2);
            blue = (blue << 3) | (blue >> 2);
            green = (green << 3) | (green >> 2);


            const uint32_t full_color = blue | (green << 8) | (red << 16);

            lut[col] = full_color | 0xff000000;
        }

        return lut;
    }

};

}