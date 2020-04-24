#include <gba/gba.h>

namespace gameboyadvance
{

Display::Display(GBA &gba) : mem(gba.mem), cpu(gba.cpu)
{
    screen.resize(SCREEN_WIDTH*SCREEN_HEIGHT);
}

void Display::init()
{
    std::fill(screen.begin(),screen.end(),0);
    cyc_cnt = 0; // current number of elapsed cycles
    ly = 0; // current number of cycles
    mode = display_mode::visible;
    new_vblank = false;
    disp_io.init();
}

// need to update these during vblank?
// bg2pa etc not sure how they work
void Display::load_reference_point_regs()
{

}

// renderer helper functions
uint16_t Display::read_palette(uint32_t pal_num,uint32_t idx)
{
    return mem.handle_read<uint16_t>(mem.pal_ram,(0x20*pal_num)+idx*2);        
}



void Display::read_tile(uint32_t tile[],bool col_256,uint32_t base,uint32_t pal_num,uint32_t tile_num, uint32_t y,bool x_flip, bool y_flip)
{
    uint32_t tile_y = y % 8;
    tile_y = y_flip? 7-tile_y : tile_y;

    // each tile accounts for 8 vertical pixels but is 2 bytes long
    uint32_t addr = base+(tile_num*0x20) + (tile_y * 4); 


    if(col_256)
    {
        throw std::runtime_error("256 color unimpl!");
    }

    else
    {

        int x_pix = x_flip? 8 : 0;
        int x_step = x_flip? -2 : +2;
        for(int x = 0; x < 8; x += 2, x_pix += x_step)
        {
            // read out the color indexs from the tile
            uint32_t tile_offset = (x_pix / 2);
            uint8_t tile_data = mem.handle_read<uint8_t>(mem.vram,addr+tile_offset);
            uint32_t idx1 =  tile_data & 0xf;
            uint32_t idx2 = (tile_data >> 4) & 0xf;

            // read out the colors
            uint16_t color1 = read_palette(pal_num,idx1);
            uint16_t color2 = read_palette(pal_num,idx2);

            // convert and smash them to the screen
            tile[x] = convert_color(color1);
            tile[x+1] = convert_color(color2);
        }
    }
}



void Display::render_text(int id)
{

    const auto &cnt = disp_io.bg_cnt[id];
    const uint32_t bg_tile_data_base = cnt.char_base_block * 0x4000;
    uint32_t bg_map_base =  cnt.screen_base_block * 0x800;
    const uint32_t size = cnt.screen_size;  // <-- need to take this more into account!


    // 256 color one pal 8bpp? or 16 color 16 pal 4bpp 
    const bool col_256 = cnt.col_256;
        



    uint32_t scroll_x = disp_io.bg_offset_x[id].offset;
    uint32_t scroll_y = disp_io.bg_offset_y[id].offset;

    uint32_t line = (ly + scroll_y) % 512;

    // what is the start tiles
    uint32_t map_x = scroll_x / 8; 
    uint32_t map_y = line / 8;


    // add the current y offset to the base for this line
    // 32 by 32 map so it wraps around again at 32 
    bg_map_base += (map_y % 0x20) * 64; // (2 * 32);
            

    uint32_t tile_data[8];

    uint32_t x_drawn = 0; // how many pixels did we draw this time?
    for(uint32_t x = 0; x < SCREEN_WIDTH; x += x_drawn)
    {

        // 8 for each map but each map takes 2 bytes
        // its 32 by 32 so we want it to wrap back around
        // at that point
        uint32_t bg_map_offset = (map_x++ % 0x20) * 2; 


        uint32_t x_pos = (x + scroll_x) % 512;

        // if we are at greater than 256 x or y
        // we will be in a higher map than the initial
        // and thus must add an offset to the correct 
        // screen (may be a better way to do this)
        switch(size)
        {
            case 1: // 512 by 256
            {
                bg_map_offset += x_pos> 255 ? 0x800 : 0;
                break;
            }

            case 2: // 256 by 512
            {
                bg_map_offset += line > 255 ? 0x800 : 0;
                break;                        
            }

            case 3: // 512 by 512
            {
                bg_map_offset += line > 255? 0x1000 : 0;
                bg_map_offset += x_pos > 255? 0x800 : 0;
                break;                        
            }
        }

        // read out the bg entry and rip all the information we need about the tile
        uint16_t bg_map_entry = mem.handle_read<uint16_t>(mem.vram,bg_map_base+bg_map_offset);
                    

        bool x_flip = is_set(bg_map_entry,10);
        bool y_flip = is_set(bg_map_entry,11);

        uint32_t tile_num = bg_map_entry & 0x1ff; 
        uint32_t pal_num = (bg_map_entry >> 12) & 0xf;


        read_tile(tile_data,col_256,bg_tile_data_base,pal_num,tile_num,line,x_flip,y_flip);

                



                
        // finally smash it to the screen probably a nicer way to do the last part :)
        uint32_t tile_offset = x_pos % 8;

        uint32_t *buf = &tile_data[tile_offset];
        int pixels_to_draw = 8 - tile_offset;

        for(int i = 0; i < pixels_to_draw; i++)
        {
            if(x + i >= SCREEN_WIDTH)
            { 
                break;
            }
            screen[(ly*SCREEN_WIDTH)+x+i] = buf[i];
        }
        x_drawn = pixels_to_draw;           
    }    
}


// for frontend debugging

void Display::render_palette(uint32_t *palette, size_t size)
{
    for(size_t i = 0; i < size; i++)
    {
        palette[i] = convert_color(mem.handle_read<uint16_t>(mem.pal_ram,i*2));
    }   
}

void Display::render()
{
    const auto disp_cnt = disp_io.disp_cnt;
    const int render_mode = disp_cnt.bg_mode; 



    switch(render_mode)
    {

        case 0x0: // text mode
        {
            for(int i = 0; i < 4; i++)
            {
                if(disp_cnt.bg_enable[i]) // if bg enabled!
                {
                    render_text(i);
                }
            }
            
            break;
        }


        case 0x2: // bg mode 2
        {
            for(int i = 2; i < 4; i++)
            {
                if(disp_cnt.bg_enable[i]) // if bg enabled!
                {
                    render_text(i);
                }                
            }
            break;
        }

        case 0x3: // bg mode 3 
        { 
            // what is the enable for this?
            for(uint32_t x = 0; x < SCREEN_WIDTH; x++)
            {
                uint32_t c = convert_color(mem.handle_read<uint16_t>(mem.vram,(ly*SCREEN_WIDTH*2)+x*2));
                screen[(ly*SCREEN_WIDTH)+x] = c;
            }
            break;
        }


        case 0x4: // mode 4 (does not handle scrolling)
        {
            // what is the enable for this
            for(uint32_t x = 0; x < SCREEN_WIDTH; x++)
            {
                uint8_t idx = mem.vram[(ly*SCREEN_WIDTH)+x];
                uint16_t color = mem.handle_read<uint16_t>(mem.pal_ram,(idx*2));
                uint32_t c = convert_color(color);
                screen[(ly*SCREEN_WIDTH)+x] = c;
            }
            break;
        }

/*
        case 0x5: // same as mode 3 but lower screen size?
        {
            for(uint32_t x = 0; x < SCREEN_WIDTH; x++)
            {
                uint32_t c = convert_color(mem.handle_read<uint16_t>(mem.vram,(ly*SCREEN_WIDTH*2)+x*2));
                screen[ly][x] = c;
            }
            break;            
        }
*/
        default: // mode ?
        {
            auto err = fmt::format("unknown ppu mode {:08x}\n",render_mode);
            throw std::runtime_error(err);
        }
    }
}





void Display::advance_line()
{

    // if in vdraw render the line
    if(ly < 160)
    {
        render();
    }


    ly++;

    auto &disp_stat = disp_io.disp_stat;

    // need to fire an interrupt here if enabled
    if(ly == disp_stat.lyc)
    {
        // set the v counter flag
        disp_stat.lyc_hit = true;

        if(disp_stat.lyc_irq_enable)
        {
            cpu.request_interrupt(interrupt::vcount);
        }

    }

    else
    {
        disp_stat.lyc_hit = false;
    }

    // exit hblank
    disp_stat.hblank = false;
    cyc_cnt = 0; // reset cycle counter
}




// not 100% sure when interrupts are reqed
void Display::tick(int cycles)
{
    cyc_cnt += cycles;

    switch(mode)
    {
        case display_mode::visible:
        {
            if(cyc_cnt >= 960)
            {
                // enter hblank
                disp_io.disp_stat.hblank = false;
                mode = display_mode::hblank;

                // if hblank irq enabled
                if(disp_io.disp_stat.hblank_irq_enable)
                {
                    cpu.request_interrupt(interrupt::hblank);
                }
                mem.dma.handle_dma(dma_type::hblank);



                if(ly >= 2)
                {
                    mem.dma.handle_dma(dma_type::video_capture);
                }

            }
            break;
        }

        case display_mode::hblank:
        {
            // end of line
            if(cyc_cnt >= 1232)
            {
                advance_line();

                if(ly == 160) // 160 we need to vblank
                {
                    mode = display_mode::vblank;
                    disp_io.disp_stat.vblank = true;

                    // if vblank irq enabled
                    if(disp_io.disp_stat.vblank_irq_enable)
                    {
                        cpu.request_interrupt(interrupt::vblank);
                    }
                    mem.dma.handle_dma(dma_type::vblank);
                }

                else
                {
                    mode = display_mode::visible;
                }
            }
            break;
        }

        case display_mode::vblank:
        {


            // inc a line
            if(cyc_cnt >= 1232)
            {
                advance_line();
                if(ly == 228)
                {
                    // exit vblank
                    new_vblank = true;
                    mode = display_mode::visible;
                    disp_io.disp_stat.vblank = false;
                    ly = 0;
                }
            }


            else if(cyc_cnt >= 960) // hblank is still active even in vblank
            {
                // enter hblank (dont set the internal mode here)
                disp_io.disp_stat.hblank = true;

                // does the hblank irq & dma fire during vblank?
                if(disp_io.disp_stat.hblank_irq_enable)
                {
                    cpu.request_interrupt(interrupt::hblank);
                }
                mem.dma.handle_dma(dma_type::hblank);
            }

            break;
        }
    }
}

}