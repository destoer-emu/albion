#include <gba/gba.h>

namespace gameboyadvance
{

// get devkitpro on linux
// port our lyc test to asm (text printing is all)
// and get a proper 3ds setup
// and write some intr and dma tests amongst others


/*
    fix rendering issues,baisc sprite, window impl, affine transforms
    fix our dma sound, then unstub the memory writes
*/


// see 8 bit writes to video memory for doom bug


Display::Display(GBA &gba) : mem(gba.mem), cpu(gba.cpu)
{
    screen.resize(SCREEN_WIDTH*SCREEN_HEIGHT);
    bg_lines.resize(4);
    for(auto &x: bg_lines)
    {
        x.resize(SCREEN_WIDTH);
    }
}

void Display::init()
{
    std::fill(screen.begin(),screen.end(),0);
    cyc_cnt = 0; // current number of elapsed cycles
    ly = 0;
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



void Display::read_tile(TileData tile[],bool col_256,uint32_t base,uint32_t pal_num,uint32_t tile_num, uint32_t y,bool x_flip, bool y_flip)
{
    uint32_t tile_y = y % 8;
    tile_y = y_flip? 7-tile_y : tile_y;



    // 8bpp 
    if(col_256)
    {
        // each tile accounts for 8 vertical pixels but is 64 bytes long
        const uint32_t addr = base+(tile_num*0x40) + (tile_y * 8); 

        int x_pix = x_flip? 7 : 0;
        const int x_step = x_flip? -1 : +1;
        for(int x = 0; x < 8; x++, x_pix += x_step)
        {
            
            const uint8_t tile_data = mem.handle_read<uint8_t>(mem.vram,addr+x_pix);

            tile[x].col_num = tile_data;
            // set pal as zero so that the col num is the sole indexer
            tile[x].pal_num = 0;
        }

    }

    //4bpp
    else
    {

        // each tile accounts for 8 vertical pixels but is 32 bytes long
        const uint32_t addr = base+(tile_num*0x20) + (tile_y * 4); 

        int x_pix = x_flip? 6 : 0;
        const int x_step = x_flip? -2 : +2;

        // depending on x flip we need to swap the nibbles
        // ie with no xflip we need to use the lower nibble first
        // then shift down the 2nd nibble out of the byte
        // when we are x flipping the 1st part of the data
        // will be in the higher part of the  byte
        // as we are reading it backwards
        const int shift_one = x_flip << 2;
        const int shift_two = !x_flip << 2;

        for(int x = 0; x < 8; x += 2, x_pix += x_step)
        {
            // read out the color indexs from the tile
            const uint32_t tile_offset = (x_pix / 2);
            const uint8_t tile_data = mem.handle_read<uint8_t>(mem.vram,addr+tile_offset);

            const uint32_t idx1 = (tile_data >> shift_one) & 0xf;
            const uint32_t idx2 = (tile_data >> shift_two) & 0xf;

            tile[x].col_num = idx1; 
            tile[x].pal_num = pal_num;

            tile[x+1].col_num = idx2; 
            tile[x+1].pal_num = pal_num;
        }
    }
}



void Display::render_text(int id)
{

    const auto &cnt = disp_io.bg_cnt[id];
    const uint32_t bg_tile_data_base = cnt.char_base_block * 0x4000;
    uint32_t bg_map_base =  cnt.screen_base_block * 0x800;
    const uint32_t size = cnt.screen_size;  


    // 256 color one pal 8bpp? or 16 color 16 pal 4bpp 
    const bool col_256 = cnt.col_256;
        



    const uint32_t scroll_x = disp_io.bg_offset_x[id].offset;
    const uint32_t scroll_y = disp_io.bg_offset_y[id].offset;

    // modulo for x and y should probably depend on the size of bg map
    const uint32_t line = (ly + scroll_y) % 512;

    // what is the start tiles
    uint32_t map_x = scroll_x / 8; 
    const uint32_t map_y = line / 8;


    // add the current y offset to the base for this line
    // 32 by 32 map so it wraps around again at 32 
    bg_map_base += (map_y % 0x20) * 64; // (2 * 32);
            

    TileData tile_data[8];

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
        
            // default dont care :P
            case 0: // 256 by 256
            {
                break;
            }

            case 1: // 512 by 256
            {
                bg_map_offset += x_pos > 255 ? 0x800 : 0;
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
        const uint16_t bg_map_entry = mem.handle_read<uint16_t>(mem.vram,bg_map_base+bg_map_offset);

        // todo if we have the same tile ident dont bother refetching it  

        const bool x_flip = is_set(bg_map_entry,10);
        const bool y_flip = is_set(bg_map_entry,11);

        const uint32_t tile_num = bg_map_entry & 0x3ff; 
        const uint32_t pal_num = (bg_map_entry >> 12) & 0xf;

        read_tile(tile_data,col_256,bg_tile_data_base,pal_num,tile_num,line,x_flip,y_flip);
        



                
        // finally smash it to the screen probably a nicer way to do the last part :)
        // so we dont have to recopy it but ah well we can fix this later :D
        // probably best is to do what we do on the gb and just render an extra tile either side
        // and scroll it in when we actually push it to the screen 
        uint32_t tile_offset = x_pos % 8;

        TileData *buf = &tile_data[tile_offset];
        int pixels_to_draw = 8 - tile_offset;

        for(int i = 0; i < pixels_to_draw; i++)
        {
            if(x + i >= SCREEN_WIDTH)
            { 
                break;
            }
            bg_lines[id][x+i] = buf[i];
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
            for(int bg = 0; bg < 4; bg++)
            {
                if(disp_cnt.bg_enable[bg]) // if bg enabled!
                {
                    render_text(bg);
                }
            }
            

            // employ painters algortihm for now
            // im not sure if we should just reverse iterate over it
            // and find the first non transparent pixel
            // also we should ideally cache this bg_priority array on bg_cnt writes

            struct BgPriority
            {
                int bg;
                int priority;
            };

            BgPriority bg_priority[4];

            for(int i = 0; i < 4; i++)
            {
                bg_priority[i].bg = i;
                bg_priority[i].priority = disp_io.bg_cnt[i].priority;
            }

            // reverse sort so highest priority is at the end of the array
            std::sort(&bg_priority[0],&bg_priority[4],
            [](const BgPriority &a, const BgPriority &b)
            {
                // if they have equal priority the lower bg idx wins
                if(a.priority == b.priority)
                {
                    return b.bg > a.bg;
                }

                // else by the bg_cnt priority
                return a.priority > b.priority;
            });


            // probably need to split this step
            // and the color conversion for when we attempt to overlay sprites on it
            // but ignore this for now 
            for(size_t x = 0; x < SCREEN_WIDTH; x++)
            {
                // default to backdrop color
                screen[(ly*SCREEN_WIDTH)+x] = convert_color(read_palette(0,0));
                for(int i = 0; i < 4; i++)
                {
                    int bg = bg_priority[i].bg;
                    if(disp_cnt.bg_enable[bg]) // if bg enabled!
                    {
                        const auto &data = bg_lines[bg][x];
                        if(data.col_num != 0)
                        {
                            const uint32_t full_color = convert_color(read_palette(data.pal_num,data.col_num));
                            screen[(ly*SCREEN_WIDTH)+x] = full_color;
                        }
                    }
                }
            }
                
            break;
        }

/*
        // very buggy needs proper impl
        case 0x1: // text mode
        {
            for(int bg = 0; bg < 4; bg++)
            {
                if(disp_cnt.bg_enable[bg]) // if bg enabled!
                {
                    render_text(bg);
                }
            }
            

            // employ painters algortihm for now
            // im not sure if we should just reverse iterate over it
            // and find the first non transparent pixel
            // also we should ideally cache this bg_priority array on bg_cnt writes

            struct BgPriority
            {
                int bg;
                int priority;
            };

            BgPriority bg_priority[4];

            for(int i = 0; i < 4; i++)
            {
                bg_priority[i].bg = i;
                bg_priority[i].priority = disp_io.bg_cnt[i].priority;
            }

            // reverse sort so highest priority is at the end of the array
            std::sort(&bg_priority[0],&bg_priority[4],
            [](const BgPriority &a, const BgPriority &b)
            {
                // if they have equal priority the lower bg idx wins
                if(a.priority == b.priority)
                {
                    return b.bg > a.bg;
                }

                // else by the bg_cnt priority
                return a.priority > b.priority;
            });



            for(size_t x = 0; x < SCREEN_WIDTH; x++)
            {
                // default to backdrop color
                screen[(ly*SCREEN_WIDTH)+x] = convert_color(read_palette(0,0));
                for(int i = 0; i < 4; i++)
                {
                    int bg = bg_priority[i].bg;
                    if(disp_cnt.bg_enable[bg]) // if bg enabled!
                    {
                        const auto &data = bg_lines[bg][x];
                        if(data.col_num != 0)
                        {
                            const uint32_t full_color = convert_color(read_palette(data.pal_num,data.col_num));
                            screen[(ly*SCREEN_WIDTH)+x] = full_color;
                        }
                    }
                }
            }
                
            break;
        }
*/

/*      need to impl properly
        case 0x2: // bg mode 2
        {
            for(int bg = 2; bg < 4; bg++)
            {
                if(disp_cnt.bg_enable[bg]) // if bg enabled!
                {
                    render_text(bg);
                }                
            }



            // employ painters algortihm for now
            // im not sure if we should just reverse iterate over it
            // and find the first non transparent pixel
            // also we should ideally cache this bg_priority array on bg_cnt writes

            struct BgPriority
            {
                int bg;
                int priority;
            };

            BgPriority bg_priority[2];

            for(int i = 0; i < 2; i++)
            {
                bg_priority[i].bg = i;
                bg_priority[i].priority = disp_io.bg_cnt[i].priority;
            }

            // reverse sort so highest priority is at the end of the array
            std::sort(&bg_priority[0],&bg_priority[2],
            [](const BgPriority &a, const BgPriority &b)
            {
                // if they have equal priority the lower bg idx wins
                if(a.priority == b.priority)
                {
                    return b.bg > a.bg;
                }

                // else by the bg_cnt priority
                return a.priority > b.priority;
            });


            for(size_t x = 0; x < SCREEN_WIDTH; x++)
            {
                // default to backdrop color
                screen[(ly*SCREEN_WIDTH)+x] = convert_color(read_palette(0,0));
                for(int i = 2; i < 4; i++)
                {
                    int bg = bg_priority[i-2].bg;
                    if(disp_cnt.bg_enable[bg]) // if bg enabled!
                    {
                        const auto &data = bg_lines[bg][x];
                        if(data.col_num != 0)
                        {
                            const uint32_t full_color = convert_color(read_palette(data.pal_num,data.col_num));
                            screen[(ly*SCREEN_WIDTH)+x] = full_color;
                        }
                    }
                }
            }

            break;
        }
*/

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


// not asserted on irq enable changes
// thanks fleroviux (vcountirq.gba)
void Display::update_vcount_compare()
{
    auto &disp_stat = disp_io.disp_stat;

    // if the compare bit was previously off and is now on
    // fire an interrupt
    const bool cur = ly == disp_stat.lyc;


    // if rapid lyc writes happen desetting and setting the bit
    // this can fire multiple times on the same line
    // see lyc_midline_rapid.gba
    if(disp_stat.lyc_irq_enable && !disp_stat.lyc_hit && cur)
    {
        cpu.request_interrupt(interrupt::vcount);
    }

    // set the v counter flag
    disp_stat.lyc_hit = cur;
}

void Display::advance_line()
{
    ly++;


    // if there is a video capture dma turn it off
    // it does not repeat the next frame it gets disabled
    // until it is turned back on
    if(ly == 162)
    {
        mem.dma.turn_off_video_capture();
    }

    // >= 2 req a video capture dma
    else if(ly >= 2)
    {
        mem.dma.handle_dma(dma_type::video_capture);
    }




    update_vcount_compare();

    // exit hblank
    disp_io.disp_stat.hblank = false;
    cyc_cnt = 0; // reset cycle counter
}




void Display::tick(int cycles)
{
    cyc_cnt += cycles;

    switch(mode)
    {
        case display_mode::visible:
        {
            // enter hblank 
            if(cyc_cnt >= 960)
            {


                // if in vdraw render the line
                if(ly < 160)
                {
                    render();
                }

                mode = display_mode::hblank;


                // flag should not get set to later

                // if hblank irq enabled
                if(disp_io.disp_stat.hblank_irq_enable)
                {
                    cpu.request_interrupt(interrupt::hblank);
                }
                mem.dma.handle_dma(dma_type::hblank);
            }
            break;
        }

        case display_mode::hblank:
        {
            // end of line
            if(cyc_cnt >= 1232)
            {
                advance_line();

                // 160 we need to enter vblank
                if(ly == 160) 
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

            else if(cyc_cnt >= 1006)
            {
                // flag should be set later at 1006 cycles because of oam search?
                disp_io.disp_stat.hblank = true;
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
                    ly = 0;
                }
                // not set on line 227
                else if(ly == 227)
                {
                    disp_io.disp_stat.vblank = false;
                }
            }




            // hblank is still active even in vblank
            else if(cyc_cnt >= 1006 && !disp_io.disp_stat.hblank) 
            {
                // enter hblank (dont set the internal mode here)
                disp_io.disp_stat.hblank = true;

                // should the irq be delayed as well?
                if(disp_io.disp_stat.hblank_irq_enable)
                {
                    cpu.request_interrupt(interrupt::hblank);
                }

                // dma does not happen here
            }
            break;
        }
    }
}

}