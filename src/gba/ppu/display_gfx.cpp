#include <gba/gba.h>

namespace gameboyadvance
{
// renderer helper functions
uint16_t Display::read_bg_palette(uint32_t pal_num,uint32_t idx)
{
    return mem.handle_read<uint16_t>(mem.pal_ram,(0x20*pal_num)+idx*2);        
}


uint16_t Display::read_obj_palette(uint32_t pal_num,uint32_t idx)
{
    // 0x200 base for sprites into pal ram
    return mem.handle_read<uint16_t>(mem.pal_ram,0x200+(0x20*pal_num)+(idx*2));        
}


void Display::read_tile(TileData tile[],unsigned int bg,bool col_256,uint32_t base,uint32_t pal_num,uint32_t tile_num, uint32_t y,bool x_flip, bool y_flip)
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
            tile[x].bg = bg;
        }

    }

    //4bpp
    else
    {

        // each tile accounts for 8 vertical pixels but is 32 bytes long
        const uint32_t addr = base+(tile_num*0x20) + (tile_y * 4); 

        int x_pix = x_flip? 3 : 0;
        const int x_step = x_flip? -1 : +1;

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
            const uint8_t tile_data = mem.handle_read<uint8_t>(mem.vram,addr+x_pix);

            const uint32_t idx1 = (tile_data >> shift_one) & 0xf;
            const uint32_t idx2 = (tile_data >> shift_two) & 0xf;

            tile[x].col_num = idx1; 
            tile[x].pal_num = pal_num;
            tile[x].bg = bg;

            tile[x+1].col_num = idx2; 
            tile[x+1].pal_num = pal_num;
            tile[x+1].bg = bg;
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

        read_tile(tile_data,id,col_256,bg_tile_data_base,pal_num,tile_num,line,x_flip,y_flip);
        



                
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



void Display::render_mode_zero()
{

    const auto disp_cnt = disp_io.disp_cnt;

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
            return a.bg > b.bg;
        }

        // else by the bg_cnt priority
        return a.priority > b.priority;
    });


    const TileData backdrop(0,0,0);
    for(size_t x = 0; x < SCREEN_WIDTH; x++)
    {
        // default to backdrop color
        bg_line[x] = backdrop;
        for(int i = 0; i < 4; i++)
        {
            const auto bg = bg_priority[i].bg;
            // if bg enabled!
            // should move this check into the sort so we completly ignore
            // drawing it ideally
            if(disp_cnt.bg_enable[bg]) 
            {
                const auto data = bg_lines[bg][x];
                if(data.col_num != 0)
                {
                    bg_line[x] = data;
                }
            }
        }
    }

}

/*
// struct to hold all the obj information
// may or may not need this
struct Obj
{
    // attr 0
    int y;
    bool scaling;
    union
    {
        bool double_size;
        bool obj_disable;
    };
    int mode;
    bool mosaic;
    bool color;
    int shape;

    // attr 1
    int x_cord;

    // if scaling is set
    int rotation_param;


    // if scanling is not set
    bool x_flip;
    bool y_flip;

    int obj_size;

    // attr 2
    int tile_num;
    int priority;

    // unused in color mode
    int palette;
};
*/

// need to get this handling priority and sizes
// amongst other things
// which we will probably just panic on for now
// we need to render this to a buf
// and then overlay it with the screen buffer
// im not sure how "transparency" is handled between sprite and gb
// may only need to pass the wether or not we are in a bitmap
// instead of the mode
void Display::render_sprites(int mode)
{

    TileData lose_bg(0,0,0);
    // make all of the line lose
    // until something is rendred over it
    std::fill(sprite_line.begin(),sprite_line.end(),lose_bg);


    const bool is_bitmap = mode >= 3;

    for(int i = 127; i >= 0; i--)
    {
        int obj_idx = i * 8;
        

        // determine if the object intersects with this line
        // for now we will assume its 8 by 8 during this
        // and handle the size later

        const auto attr0 = mem.handle_read<uint16_t>(mem.oam,obj_idx);
        const auto attr1 = mem.handle_read<uint16_t>(mem.oam,obj_idx+2);
        const auto attr2 = mem.handle_read<uint16_t>(mem.oam,obj_idx+4);


        // to handle drawing from off screen vertically and horizontally
        // on the upper and left hand sides

        const unsigned int y_cord = attr0 & 0xff;
        const bool scale = is_set(attr0,8);

        // should check mosaic by here but we will just ignore it for now


        // from here on out we assume this isnt using affine sprites
        if(scale)
        {
            //puts("scaling/rotation unsupported!");
            //exit(1);
        }

        // disable bit
        if(is_set(attr0,9))
        {
            continue;
        }

        const int obj_mode = (attr0 >> 10) & 0x3;

        if(obj_mode != 0)
        {
            //printf("unhandled obj mode %d\n",obj_mode);
            //exit(1);
        }

        // prohibited is this ignored on hardware
        // or does it behave like another?
        if(obj_mode == 3)
        {
            continue;
        }

        const int shape = (attr0 >> 14) & 0x3;

        // prohibited is this ignored on hardware
        // or does it behave like another?
        if(shape == 3)
        {
            continue;
        }


        const int obj_size = (attr1 >> 14) & 0x3;

        static constexpr int x_size_lookup[3][4] = 
        {
            {8,16,32,64},
            {16,32,32,64},
            {8,8,16,32}
        };

        static constexpr int y_size_lookup[3][4] = 
        {
            {8,16,32,64},
            {8,8,16,32},
            {16,32,32,64}
        };



        // replace with a lookup table for shapes later
        const unsigned int y_size = y_size_lookup[shape][obj_size];
        unsigned int x_size = x_size_lookup[shape][obj_size];
        const unsigned int y_max = y_size-1;




        bool line_overlap;

        if(y_cord < SCREEN_HEIGHT)
        {
            line_overlap = ((y_cord + y_size) > ly && y_cord <= ly);
        }

        // overflowed from 255
        else
        {
            // by definiton it is allways greater than ly before it overflows
            uint8_t y_end = (y_cord + y_size) & 255;
            line_overlap = y_end >= ly && y_end < SCREEN_HEIGHT; 
        }

        if(!line_overlap)
        {
            continue;
        }

        const bool color = is_set(attr0,13);

        if(color)
        {
            puts("256 sprites unsupported!");
            exit(1);
        }

        // assume palette



        // current x cords greater than screen width are handled in the decode loop
        // by ignoring them until they are in range
        const unsigned int x_cord = attr1 & 511;

        // if cordinate out of screen bounds and does not wrap around
        // then we dont care
        if(x_cord >= SCREEN_WIDTH && x_cord + x_size < 512)
        {
            continue;
        }


        // assume no scaling for now
        const bool x_flip = is_set(attr1,12);
        const bool y_flip = is_set(attr1,13);


        const unsigned int tile_num = attr2 & 0x3ff;
        const unsigned int pal =  (attr2 >> 12) & 0xf;
        const unsigned int priority = (attr2 >> 10) & 3;


        // bitmap modes starts at  0x14000 instead of 0x10000
        // because of bg map tiles below this are simply ignored
        if(is_bitmap && tile_num < 512)
        {
            continue;
        }


        const unsigned int y_pix = y_flip?  y_max - ((ly-y_cord) & y_max) : ((ly-y_cord) & y_max);

        // each tile accounts for 8 vertical pixels but is 32 bytes long
        uint32_t addr;

        // 1d object mapping
        if(disp_io.disp_cnt.obj_vram_mapping)
        {
            // base + ((tile_num + (tile_map_y*x_map_width)) * TILE_SIZE) + (tile_line * LINE_SIZE)
            addr = 0x10000 + ((tile_num + ((x_size / 8) * (y_pix / 8)) ) * 8 * 4) + ((y_pix % 8) * 4);
        }

        // 2d object mapping
        // in 4bpp 1024 tiles split into 32 by 32
        else
        {
            // base + ((tile_num + (tile_map_y*x_map_width)) * TILE_SIZE) + (tile_line * LINE_SIZE)
            addr = 0x10000 + ((tile_num + (32 * (y_pix / 8)) ) * 8 * 4) + ((y_pix % 8) * 4);            
        }

        unsigned int x_pix = x_flip? (x_size/2)-1 : 0;
        const int x_step = x_flip? -1 : +1;

        // depending on x flip we need to swap the nibbles
        // ie with no xflip we need to use the lower nibble first
        // then shift down the 2nd nibble out of the byte
        // when we are x flipping the 1st part of the data
        // will be in the higher part of the  byte
        // as we are reading it backwards
        const int shift_one = x_flip << 2;
        const int shift_two = !x_flip << 2;

        for(unsigned int x = 0; x < x_size; x += 2, x_pix += x_step)
        {
            // read out the color indexs from the tile
            // x_byte_tile_offset + (tile_map_x * tile_size)
            const uint32_t x_data_offset = (x_pix % 4) + ((x_pix / 4) * 8 * 4);
            const uint8_t tile_data = mem.handle_read<uint8_t>(mem.vram,addr+x_data_offset);

            const uint32_t idx1 = (tile_data >> shift_one) & 0xf;
            const uint32_t idx2 = (tile_data >> shift_two) & 0xf;
    
            const uint32_t x_offset = (x_cord + x) & 511;

            // probably a nicer way to do this but this is fine for now
            if(x_offset >= SCREEN_WIDTH || x_offset+1 >= SCREEN_WIDTH)
            {
                continue;
            }

            // may be better to just have an array with sprites
            // that has arrays of the pixels as we are doing many extra comps
            // and hold a length that takes account of the screen cut off
            // when we merge the bg and sprites

            // if our cur pixel is transparent dont write it
            if(idx1 != 0)
            {
                sprite_line[x_offset].col_num = idx1;
                sprite_line[x_offset].pal_num = pal;
                sprite_line[x_offset].bg = priority;
            }

            if(idx2 != 0)
            {
                sprite_line[x_offset+1].col_num = idx2;
                sprite_line[x_offset+1].pal_num = pal;
                sprite_line[x_offset+1].bg = priority;
            }
        }

    }
}


void Display::render()
{
    const auto disp_cnt = disp_io.disp_cnt;
    const int render_mode = disp_cnt.bg_mode; 


    const bool is_bitmap = render_mode >= 3;

    switch(render_mode)
    {

        case 0x0: // text mode
        {
            render_mode_zero();
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
                screen[(ly*SCREEN_WIDTH)+x] = convert_color(read_bg_palette(0,0));
                for(int i = 0; i < 4; i++)
                {
                    int bg = bg_priority[i].bg;
                    if(disp_cnt.bg_enable[bg]) // if bg enabled!
                    {
                        const auto &data = bg_lines[bg][x];
                        if(data.col_num != 0)
                        {
                            const uint32_t full_color = convert_color(read_bg_palette(data.pal_num,data.col_num));
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
                screen[(ly*SCREEN_WIDTH)+x] = convert_color(read_bg_palette(0,0));
                for(int i = 2; i < 4; i++)
                {
                    int bg = bg_priority[i-2].bg;
                    if(disp_cnt.bg_enable[bg]) // if bg enabled!
                    {
                        const auto &data = bg_lines[bg][x];
                        if(data.col_num != 0)
                        {
                            const uint32_t full_color = convert_color(read_bg_palette(data.pal_num,data.col_num));
                            screen[(ly*SCREEN_WIDTH)+x] = full_color;
                        }
                    }
                }
            }

            break;
        }
*/

        // currently testing sprites under the assumptions of txt_obj
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

    render_sprites(render_mode);

    // does a lesser priority obj pixel 
    // draw over a transparent bg pixel ?


    // now to merge sprite and bg
    // this needs to be split off into its own function
    if(is_bitmap)
    {
        // check directly against the screen
        // if sprite loses 
        // (ideally we would not render the bitmap at all if has lost priority)
        for(unsigned int x = 0; x < SCREEN_WIDTH; x++)
        {
            const auto s = sprite_line[x];
            // col number zero is transparent
            if(s.col_num != 0)
            {
                screen[(ly*SCREEN_WIDTH) + x] = convert_color(read_obj_palette(s.pal_num,s.col_num));
            }
        }
    }

    else
    {
        // compare sprites against our final bg line
        // and convert colors accordingly
        for(unsigned int x = 0; x < SCREEN_WIDTH; x++)
        {
            const auto s = sprite_line[x];
            const auto b = bg_line[x];

            // sprite has higher priority and is not transparent
            if((s.bg <= disp_io.bg_cnt[b.bg].priority || b.col_num == 0) && s.col_num != 0)
            {
                screen[(ly*SCREEN_WIDTH) + x] = convert_color(read_obj_palette(s.pal_num,s.col_num));
            }

            else
            {
                screen[(ly*SCREEN_WIDTH) + x] = convert_color(read_bg_palette(b.pal_num,b.col_num));
            }

        }
    }

}

}