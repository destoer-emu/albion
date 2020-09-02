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


void Display::render_affine(int id)
{
    if(!disp_io.disp_cnt.bg_enable[id])
    {
        return;
    }

    const auto bg_cnt = disp_io.bg_cnt[id];
    const auto bg_tile_data_base = bg_cnt.char_base_block * 0x4000;
    const auto bg_map_base = bg_cnt.screen_base_block * 0x800;
    const auto size = bg_cnt.screen_size;

    // need to figure out the size in pixel and set of map
    // this is treated as if its one giant screen rather than multiple sections

    // for now we will just handle overflow or showing something as transparent
    // and iter thru each pixel one bit at a time

    // each one is same width and height
    static constexpr int32_t bg_size[] = {128,256,512,1024};
    const auto cord_size = bg_size[size];
    const auto map_size = cord_size / 8;
    const auto area_overflow = bg_cnt.area_overflow;

    const TileData transparent(0,0,0);

    auto &buf = bg_lines[id];

    auto &ref_point = id == 2? disp_io.bg2_ref_point : disp_io.bg3_ref_point;

    // what do i do with actual paramaters here?
    const auto &scale_param = id == 2? disp_io.bg2_scale_param : disp_io.bg3_scale_param;
    
    // unsure how internal ref points work
    auto &ref_point_x = ref_point.int_ref_point_x;
    auto &ref_point_y = ref_point.int_ref_point_y;

    for(uint32_t x = 0; x < SCREEN_WIDTH; x++)
    {

        const auto x_param = static_cast<int32_t>(x);
        const auto y_param = static_cast<int32_t>(ly); 

        // transform applied and then the displacemnt
        // origin at 0,0 of screen

        // transform cords
        int32_t x_affine = ((scale_param.a*x_param + scale_param.b*y_param) >> 8);
        int32_t y_affine = ((scale_param.c*x_param + scale_param.d*y_param) >> 8);

        // apply displacemnt
        x_affine += ref_point_x >> 8;
        y_affine += ref_point_y >> 8;

        // depending on what setting we have make pixel
        // trasparent or wrap around the x cord
        if(x_affine >= cord_size || x_affine < 0)
        {
            if(area_overflow)
            {
                x_affine &= cord_size - 1;
                if(x_affine < 0)
                {
                    x_affine += cord_size;
                }
            }

            else
            {
                buf[x] = transparent;
                continue;
            }
        }
        // same for y
        if(y_affine >= cord_size || y_affine < 0)
        {
            if(area_overflow)
            {
                y_affine &= cord_size - 1;
                if(y_affine < 0)
                {
                    y_affine += cord_size;
                }
            }

            else
            {
                buf[x] = transparent;
                continue;
            }
        }

        // get tile num from bg map
        const auto tile_num = mem.handle_read<uint8_t>(mem.vram,bg_map_base + ((y_affine / 8) * map_size) + (x_affine / 8));

        // now figure out where we are offset into the current tile and smash it into the line
        const auto tile_x = x_affine % 8;
        const auto tile_y = y_affine % 8;

        // each tile accounts for 8 vertical pixels but is 64 bytes long
        const uint32_t addr = bg_tile_data_base+(tile_num*0x40) + (tile_y * 8); 
        
        // affine is allways 8bpp
        const uint8_t tile_data = mem.handle_read<uint8_t>(mem.vram,addr+tile_x);

        buf[x].col_num = tile_data;
        // set pal as zero so that the col num is the sole indexer
        buf[x].pal_num = 0;
        buf[x].bg = id;
    }

    ref_point_x += scale_param.b >> 8;
    ref_point_y += scale_param.d >> 8;
}

void Display::render_text(int id)
{
    if(!disp_io.disp_cnt.bg_enable[id])
    {
        return;
    }

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


void Display::merge_layers()
{
    const auto disp_cnt = disp_io.disp_cnt;
    const auto render_mode = disp_cnt.bg_mode;


    // see notes below for what i think is better... 
 
    // right so we wanna impl bldcnt
    // we are gonna change up the sort of bg
    // so that we include the obj layer in the array
    // and have a source so we can easily index the priority + the bldcnt settings
    // and then just run a blend by hitting 1st target and looking for 2nd below it
    // (if we hit the bottom we have to check bldcnt we may just have this as a bottom (really high value so it allways loses))
    // (array section for ease and look into the perf implications later)
    // im not sure how we will handle obj thought because they can have a different priority
    // on each pixel whereas every other layer has to have the same priority on it
    // so more than likely on each transparency descend we will have to hard check the obj layer each time?
    // we will look into impl this tomorrow just run bldcnt and a couple roms when done
    // to check we havent messed anything up


/*
        // this should work but it involves very uncessary descends for data we may never use
        // it would be nicer if we could just find the 1st and 2nd target and test if they are adjacent
        // at any given pixel...
        // (this mehtod also means we dont need to change our current structure (infact its easier not to))


        // operation for handling bld mode 1

        // default to backdrop!

        // so instead we could just jump to highest priority of 1st target if trans
        // go to next highest 
        // (we need to check if sprite is highest as a hard code before we decsned bgs)
        // if first was bg check sprite for 2nd target
        // if its not keep going down to the 2nd target
        // if at any point we see a non transparent pixel we will just break out early
        // if its not there at all then check if backdrop is a target  
        // else we dont give a damb and just draw the damb thing
        // for the latter idealy we would just when we get the first pixel that aint trans
        // with the highest priority save it
        // at the fail point we can then just quickly dump it in place
        // (in impl we will probs just do as a default color decode)
        // ( and then overwrite with a blended one if found )

        // note that if the semi trans bit is set for an object in this postion and it wins ...
        // then we need to run the above procedure regardless of bldcnt mode


        // for bright / dark mode we simply need to just run the normal code
        // and apply a brightness calc on the color if the winning pixel is a 1st target :P

        // lets say this is the effect mode 1 handler
        const TileData backdrop(0,0,0);
        for(size_t x = 0; x < SCREEN_WIDTH; x++)
        {
            // default to backdrop color
            bg_line[x] = backdrop;
            for(unsigned int i = 0; i < lim; i++)
            {
                // this will override anything drawn by the fail case...
                if(first_target(bg) && opaque)
                {
                    // search for 2nd if not found
                    // draw
                }

                else // push this as line data
                {
                    const auto bg = bg_priority[i].bg;
                    // if bg enabled!
                    // should move this check into the sort so we completly ignore
                    // drawing it ideally
                    // need a function by here that checks the bg enable for a specific x and y cord
                    if(disp_cnt.bg_enable[bg] && bg_window_enabled(bg,x,ly)) 
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
*/



    // does a lesser priority obj pixel 
    // draw over a transparent bg pixel ?
    bool is_bitmap = render_mode >= 3;

    // ignore blending in bitmap modes for :)

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
            if(s.col_num != 0 && sprite_window_enabled(x,ly))
            {
                screen[(ly*SCREEN_WIDTH) + x] = convert_color(read_obj_palette(s.pal_num,s.col_num));
            }
        }
    }

    else
    {
        static constexpr unsigned int bg_limits[3][2] = 
        {
            {0,4},
            {0,3},
            {2,4}
        };

        // ideally id find a nicer way to split off is_bitmap so this is not required
        const auto start = bg_limits[render_mode][0];
        const auto end = bg_limits[render_mode][1];
        



        // employ painters algortihm for now
        // im not sure if we should just reverse iterate over it
        // and find the first non transparent pixel
        // also we should ideally cache this bg_priority array on bg_cnt writes

        struct BgPriority
        {
            int bg;
            int priority;
        };

        // max of four but we may end up using less
        BgPriority bg_priority[4];

        const unsigned int lim = end-start;

        for(unsigned int i = 0; i < lim; i++)
        {
            bg_priority[i].bg = i+start;
            bg_priority[i].priority = disp_io.bg_cnt[i+start].priority;
        }

        // reverse sort so highest priority is at the end of the array
        std::sort(&bg_priority[0],&bg_priority[lim],
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

        //const auto &bldcnt = disp_io.bldcnt;

        const TileData backdrop(0,0,std::numeric_limits<int>::max());

        enum class pixel_source
        {
            bg,
            obj
        };

        for(size_t x = 0; x < SCREEN_WIDTH; x++)
        {
            // assume bg wins
            auto source = pixel_source::bg;

            // default to backdrop color
            auto pixel = backdrop;

            // find first active bg pixel
            // if none are found the backdrop will win
            for(int i = lim-1; i >= 0; i--)
            {
                const auto bg = bg_priority[i].bg;

                if(disp_cnt.bg_enable[bg] && bg_window_enabled(bg,x,ly)) 
                {
                    const auto &b = bg_lines[bg][x];
                    if(b.col_num != 0)
                    {
                        pixel = b;
                        break;
                    }
                }
            }


            // sprite has priority
            // note the lower the better here
            // if equal a sprite wins
            const auto &s = sprite_line[x];
            if((s.bg <= disp_io.bg_cnt[pixel.bg].priority || pixel.col_num == 0) && sprite_window_enabled(x,ly) && s.col_num != 0)
            {
                source = pixel_source::obj;

                // sprite has the highest priority update the pixel
                pixel = s;         
            }

            const auto color = source == pixel_source::obj? read_obj_palette(pixel.pal_num,pixel.col_num) : read_bg_palette(pixel.pal_num,pixel.col_num);
            screen[(ly*SCREEN_WIDTH) + x] = convert_color(color);
        }
    }
}


// account for l > r behavior
bool window_in_range(const unsigned int c, const unsigned int w1, const unsigned int w2)
{
    // in this case the nearest screen edge to
    // the window is bound is valid
    if(w1 > w2)
    {
        return c >= w1 || c < w2;
    }

    else
    {
        return c >= w1 && c < w2;
    }
}


// this can be optimsed to lesson checks
// ideally we would just have a lookup array of bools
// that marks if something is enabled
// or split up loops on window bounds (need an array of bounds for obj window)
bool Display::bg_window_enabled(unsigned int bg, unsigned int x, unsigned int y) const
{
    // check either window is enabled
    // if not bg is enabled
    const auto dispcnt = disp_io.disp_cnt;

    // if no windows are active then out of window is not enabled
    if(!dispcnt.window0_enable && !dispcnt.window1_enable)
    {
        return true;
    }

    // figure out which is enabled at current cords
    // if both are prefer win 0

    // first check win0
    // if not enabled check win 1

    if(dispcnt.window0_enable)
    {
        if(window_in_range(x,disp_io.win0h.x1,disp_io.win0h.x2))
        {
            if(window_in_range(y,disp_io.win0v.y1,disp_io.win0v.y2))
            {
                return disp_io.win_in.bg_enable_lower[bg];
            }
        }
    }

    if(dispcnt.window1_enable)
    {
        if(window_in_range(x,disp_io.win1h.x1,disp_io.win1h.x2))
        {
            if(window_in_range(y,disp_io.win1v.y1,disp_io.win1v.y2))
            {
                return disp_io.win_in.bg_enable_upper[bg];
            }
        }
    }

    // win out
    return disp_io.win_out.bg_enable_lower[bg];
    
}

bool Display::sprite_window_enabled(unsigned int x,unsigned y) const
{
    // check either window is enabled
    // if not bg is enabled
    const auto dispcnt = disp_io.disp_cnt;

    // if no windows are active then out of window is not enabled
    if(!dispcnt.window0_enable && !dispcnt.window1_enable)
    {
        return true;
    }

    // figure out which is enabled at current cords
    // if both are prefer win 0

    // first check win0
    // if not enabled check win 1

    if(dispcnt.window0_enable)
    {
        if(window_in_range(x,disp_io.win0h.x1,disp_io.win0h.x2))
        {
            if(window_in_range(y,disp_io.win0v.y1,disp_io.win0v.y2))
            {
                return disp_io.win_in.obj_enable_lower;
            }
        }
    }

    if(dispcnt.window1_enable)
    {
        if(window_in_range(x,disp_io.win1h.x1,disp_io.win1h.x2))
        {
            if(window_in_range(y,disp_io.win1v.y1,disp_io.win1v.y2))
            {
                return disp_io.win_in.obj_enable_upper;
            }
        }
    }

    // win out
    return disp_io.win_out.obj_enable_lower;
    
}


void Display::render()
{
    const auto disp_cnt = disp_io.disp_cnt;
    const auto render_mode = disp_cnt.bg_mode; 

    switch(render_mode)
    {

        case 0x0: // text mode
        {
            //render_bg(0,4);
            for(unsigned int bg = 0; bg < 4; bg++)
            {
                render_text(bg);
            }
            break;
        }


        // needs checking
        case 0x1: // text mode
        {
            //render_bg(0,3);
            for(unsigned int bg = 0; bg < 2; bg++)
            {
                render_text(bg);
            }

            render_affine(2);

            break;
        }


/*      // needs checking
        case 0x2: // bg mode 2
        {
            //render_bg(2,4);
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

    render_sprites(render_mode);

    merge_layers();

}

}