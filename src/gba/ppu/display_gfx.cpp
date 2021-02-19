#include <gba/gba.h>

namespace gameboyadvance
{
// TODO
// fix gfx glitches in fire emblem after title screen
// metroid screen tears (allthough i think this a timing issue)
// try and fix battle start bug in emerald

// todo cache these from the color lut on pal write to directly return a uint32_t

// renderer helper functions
uint16_t Display::read_bg_palette(uint32_t pal_num,uint32_t idx)
{
    return handle_read<uint16_t>(mem.pal_ram,(0x20*pal_num)+idx*2);        
}


uint16_t Display::read_obj_palette(uint32_t pal_num,uint32_t idx)
{
    // 0x200 base for sprites into pal ram
    return handle_read<uint16_t>(mem.pal_ram,0x200+(0x20*pal_num)+(idx*2));        
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
            
            const auto tile_data = mem.vram[addr+x_pix];

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
            const uint8_t tile_data = mem.vram[addr+x_pix];

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
        const auto tile_num = mem.vram[bg_map_base + ((y_affine / 8) * map_size) + (x_affine / 8)];

        // now figure out where we are offset into the current tile and smash it into the line
        const auto tile_x = x_affine % 8;
        const auto tile_y = y_affine % 8;

        // each tile accounts for 8 vertical pixels but is 64 bytes long
        const uint32_t addr = bg_tile_data_base+(tile_num*0x40) + (tile_y * 8); 
        
        // affine is allways 8bpp
        const uint8_t tile_data = mem.vram[addr+tile_x];

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

    uint32_t old_entry = 0xffffffff;

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
            
    for(uint32_t x = 0; x < SCREEN_WIDTH+16; x += 8)
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
        const uint32_t bg_map_entry = handle_read<uint16_t>(mem.vram,bg_map_base+bg_map_offset);


        if(bg_map_entry != old_entry)
        {

            const bool x_flip = is_set(bg_map_entry,10);
            const bool y_flip = is_set(bg_map_entry,11);

            const uint32_t tile_num = bg_map_entry & 0x3ff; 
            const uint32_t pal_num = (bg_map_entry >> 12) & 0xf;


            // render a full tile but then just lie and say we rendered less
            TileData *tile_data = &bg_lines[id][x];
            read_tile(tile_data,id,col_256,bg_tile_data_base,pal_num,tile_num,line,x_flip,y_flip);
            old_entry = bg_map_entry;
        }


        // just copy our old chunk here as its the same
        else
        {
            memcpy(&bg_lines[id][x], &bg_lines[id][x-8],sizeof(TileData) * 8);
        }
    }   
 
}


uint16_t Display::get_color(const TileData &data, const pixel_source source)
{
    return source == pixel_source::obj? read_obj_palette(data.pal_num,data.col_num) : read_bg_palette(data.pal_num,data.col_num);
}


// all blend parameters are in 1.4 fixed point

inline int32_t do_blend_calc(int32_t eva, int32_t evb, int32_t color1,int32_t color2)
{
    return std::min(31,(eva * color1 + evb * color2) >> 4);
}

inline int32_t do_blend(int32_t eva, int32_t evb, int32_t color1,int32_t color2)
{
    // blend the two colors!
    // our blend param for this is 1.4 fixed point
    int r1 = color1 & 0x1f;
    int g1 = (color1 >> 5) & 0x1f;
    int b1 = (color1 >> 10) & 0x1f;

    int r2 = color2 & 0x1f;
    int g2 = (color2 >> 5) & 0x1f;
    int b2 = (color2 >> 10) & 0x1f;

    const auto r = do_blend_calc(eva,evb,r1,r2);
    const auto g = do_blend_calc(eva,evb,g1,g2);
    const auto b = do_blend_calc(eva,evb,b1,b2);
    
    return r | (g << 5) | (b << 10);
}

inline int32_t do_brighten_calc(int32_t evy, int32_t color)
{
    return color + (((31-color) * evy) >> 4);
}

inline int32_t do_brighten(int32_t evy, int32_t color)
{
    // blend the two colors!
    // our blend param for this is 1.4 fixed point
    int r1 = color & 0x1f;
    int g1 = (color >> 5) & 0x1f;
    int b1 = (color >> 10) & 0x1f;

    const auto r = do_brighten_calc(evy,r1);
    const auto g = do_brighten_calc(evy,g1);
    const auto b = do_brighten_calc(evy,b1);
    
    return r | (g << 5) | (b << 10);
}



inline int32_t do_darken_calc(int32_t evy, int32_t color)
{
    return color - ((color * evy) >> 4);
}

inline int32_t do_darken(int32_t evy, int32_t color)
{
    // blend the two colors!
    // our blend param for this is 1.4 fixed point
    int r1 = color & 0x1f;
    int g1 = (color >> 5) & 0x1f;
    int b1 = (color >> 10) & 0x1f;

    const auto r = do_darken_calc(evy,r1);
    const auto g = do_darken_calc(evy,g1);
    const auto b = do_darken_calc(evy,b1);
    
    return r | (g << 5) | (b << 10);
}

void Display::merge_layers()
{
    const auto disp_cnt = disp_io.disp_cnt;
    const auto render_mode = disp_cnt.bg_mode;


    // does a lesser priority obj pixel 
    // draw over a transparent bg pixel ?
    bool is_bitmap = render_mode >= 3;

    // ignore blending in bitmap modes for :)

    // now to merge sprite and bg
    // this needs to be split off into its own function
    // TODO handle bitmap bg enable
    if(is_bitmap)
    {
        // check directly against the screen
        // if sprite loses 
        // (ideally we would not render the bitmap at all if has lost priority)
        for(unsigned int x = 0; x < SCREEN_WIDTH; x++)
        {
            const auto s = sprite_line[x];
            // col number zero is transparent
            if(s.col_num != 0 && sprite_window_enabled(x))
            {
                screen[(ly*SCREEN_WIDTH) + x] = convert_color(read_obj_palette(s.pal_num,s.col_num));
            }
        }
    }

    else
    {
        // tuck the sort function away somewhere

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

        // ideally we would ignore comparsion for the entire line if the bg is not enabled
        struct BgPriority
        {
            int bg;
            int priority;
        };

        // max of four but we may end up using less
        BgPriority bg_priority[4];

        unsigned int lim = end-start;

        unsigned int new_lim = 0;

        for(unsigned int i = 0; i < lim; i++)
        {
            const auto bg = i+start;
            // dont bother with inactive bgs
            if(disp_cnt.bg_enable[bg]) 
            {
                bg_priority[new_lim].bg = bg;
                bg_priority[new_lim].priority = disp_io.bg_cnt[bg].priority;
                new_lim++;
            }
        }
        lim = new_lim;

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

   
        const auto &bld_cnt = disp_io.bld_cnt;

        // ok so now after we find what exsacly is the first to win
        // we can then check if 1st target
        // and then redo the search starting from it for 2nd target
        // and perform whatever effect if we need to :)
        for(size_t x = 0; x < SCREEN_WIDTH; x++)
        {
            // pull the top two opaque layers
            // ideally we would only pull one if the obj on this x cord is
            // not semi transparent
            uint16_t color1 = read_bg_palette(0,0);
            uint16_t color2 = color1;
            pixel_source source1 = pixel_source::bd;
            pixel_source source2 = pixel_source::bd;

            // presume that there is a valid bg layer
            // and we will descend to find the first and compare against sprite
            // and dump whichever in the correct slot
            // after that we can just check at the end hey is the 1st one still bd
            // if so then we have to do a extra check on the sprite being enabled
            // because it means there we no enabled bgs and we need to do this for the 2nd 
            // aswell incase there is a sprite below the 1st bg but enabled over everything else

            const auto &s = sprite_line[x];
            const auto sprite_color = read_obj_palette(s.pal_num,s.col_num);
            const bool sprite_enable = sprite_window_enabled(x);
            for(int i = lim-1; i >= 0; i--)
            {
                const auto bg = bg_priority[i].bg;
                // here we are finding out how far we are offset into the intial tile
                // as we render each tile fully in render text then just cut into it here
                const auto scx = disp_io.bg_offset_x[bg].offset & 7;
                const auto &b = bg_lines[bg][x+scx];


                // valid bg pixel now check if there is a sprite at it with <= priority
                if(bg_window_enabled(bg,x) && b.col_num != 0)
                {
                    const auto bg_priority = disp_io.bg_cnt[bg].priority;

                    // lower priority is higher, sprite wins even if its equal
                    const bool obj_win = s.col_num != 0 && sprite_enable && s.bg <= bg_priority;

                    // first time
                    if(source1 == pixel_source::bd)
                    {
                        const auto bg_color = read_bg_palette(b.pal_num,b.col_num);

                        if(obj_win)
                        {
                            color1 = sprite_color;
                            source1 = pixel_source::obj;
                            color2 = bg_color;
                            source2 = static_cast<pixel_source>(bg);
                            break;
                        }

                        else
                        {
                            color1 = bg_color;
                            source1 = static_cast<pixel_source>(bg);                        
                        }
                    }

                    // 2nd time (break out and we are done)
                    else
                    {
                        if(obj_win)
                        {
                            color2 = sprite_color;
                            source2 = pixel_source::obj;
                        }

                        else
                        {
                            color2 = read_bg_palette(b.pal_num,b.col_num);
                            source2 = static_cast<pixel_source>(bg);                        
                        }
                        break;
                    }
                }
            }

    
            if(source1 == pixel_source::bd)
            {
                // bd has no priority
                if(s.col_num != 0 && sprite_enable)
                {
                    color1 = sprite_color;
                    source1 = pixel_source::obj;
                } 
            }

            else if(source2 == pixel_source::bd)
            {
                // bd has no priority
                if(s.col_num != 0 && sprite_enable)
                {
                    color2 = sprite_color;
                    source2 = pixel_source::obj;
                } 
            }



            // special effects disabled dont care
            if(!special_window_enabled(x))
            {
                screen[(ly*SCREEN_WIDTH) + x] = convert_color(color1);
                continue;
            }


            // TODO look at metroid save for edge case with alpha blending
            // handle sfx 

            // if semi transparent object is 1st layer
            // then we need to override the mode to alpha blending
            int special_effect = bld_cnt.special_effect;
            const bool second_target_enable = bld_cnt.second_target_enable[static_cast<int>(source2)];
            // if there are overlapping layers and sprite is semi transparent
            // do alpha blend
            const bool semi_transparent = sprite_semi_transparent[x] && source1 == pixel_source::obj
                && second_target_enable;


            if(semi_transparent)
            {
                special_effect = 1;
            }
            

            // todo account for special effects window
            // and split this function off
            
            switch(special_effect)
            {
                // no special effects just slam to screen
                case 0:
                {
                    break;
                }

                // alpha blending (delayed because we handle it along with semi transparency)
                case 1:
                {
                    const bool first_target_enable = semi_transparent || bld_cnt.first_target_enable[static_cast<int>(source1)];
                    if(first_target_enable && source1 != pixel_source::bd)
                    {
                        // we have a 1st and second target now we just need to blend them :P
                        if(second_target_enable)
                        {
                            color1 = do_blend(disp_io.eva,disp_io.evb,color1,color2);
                        }
                    }
                    break;
                }


                // brighness increase 
                case 2:
                {
                    if(bld_cnt.first_target_enable[static_cast<int>(source1)])
                    {
                        color1 = do_brighten(disp_io.evy,color1);
                    }
                    break;
                }

                // brightness decrease
                case 3:
                {
                    if(bld_cnt.first_target_enable[static_cast<int>(source1)])
                    {
                        color1 = do_darken(disp_io.evy,color1);
                    }
                    break;
                }
            }

            screen[(ly*SCREEN_WIDTH) + x] = convert_color(color1);
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


// this will be called before sprites are drawn
// the obj window will overwrite onto this
// ideally we would just compute index bounds on everything
// and do loops assuming features are off but this far simpler for now
void Display::cache_window()
{
    const auto &disp_cnt = disp_io.disp_cnt;

    // if no windows are active then out of window is not enabled
    if(!disp_cnt.window0_enable && !disp_cnt.window1_enable)
    {
        std::fill(window.begin(),window.end(),window_source::out);
        return;
    }

    const bool trigger_0 = disp_cnt.window0_enable && window_0_y_triggered;
    const bool trigger_1 = disp_cnt.window1_enable && window_1_y_triggered;

    for(size_t x = 0; x < SCREEN_WIDTH; x++)
    {

        // figure out which is enabled at current cords
        // if both are prefer win 0

        // first check win0
        // if not enabled check win 1

        if(trigger_0)
        {
            if(window_in_range(x,disp_io.win0h.x1,disp_io.win0h.x2))
            {
                window[x] = window_source::zero;
                continue;  
            }
        }

        
        if(trigger_1)
        {
            if(window_in_range(x,disp_io.win1h.x1,disp_io.win1h.x2))
            {     
                window[x] = window_source::one;
                continue;
            }
        }


        window[x] = window_source::out; 
    }    
}


bool Display::bg_window_enabled(unsigned int bg, unsigned int x) const
{
    const auto &disp_cnt = disp_io.disp_cnt;

    // if no windows are active then out of window is not enabled
    if(!disp_cnt.window0_enable && !disp_cnt.window1_enable && !disp_cnt.obj_window_enable)
    {
        return true;
    }

    return disp_io.win_cnt.win_arr[static_cast<size_t>(window[x])].bg_enable[bg];
}

bool Display::sprite_window_enabled(unsigned int x) const
{
    // check either window is enabled
    // if not bg is enabled
    const auto &disp_cnt = disp_io.disp_cnt;

    // if no windows are active then out of window is not enabled
    if(!disp_cnt.window0_enable && !disp_cnt.window1_enable && !disp_cnt.obj_window_enable)
    {
        return true;
    }

    return disp_io.win_cnt.win_arr[static_cast<size_t>(window[x])].obj_enable;
}

bool Display::special_window_enabled(unsigned int x) const
{
    // check either window is enabled
    // if not bg is enabled
    const auto &disp_cnt = disp_io.disp_cnt;

    // if no windows are active then out of window is not enabled
    if(!disp_cnt.window0_enable && !disp_cnt.window1_enable && !disp_cnt.obj_window_enable)
    {
        return true;
    }

    return disp_io.win_cnt.win_arr[static_cast<size_t>(window[x])].special_enable;
}


void Display::render()
{
    const auto disp_cnt = disp_io.disp_cnt;
    const auto render_mode = disp_cnt.bg_mode; 

    // ideally we would try to cull draws
    // that are not enabled in the window
    cache_window();
    render_sprites(render_mode);

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


        // needs checking
        case 0x2: // bg mode 2
        {
            //render_bg(2,4);
            for(unsigned int bg = 2; bg < 4; bg++)
            {
                render_affine(bg);
            }
            break;
        }


        case 0x3: // bg mode 3 
        { 
            // bg 2 enable for this
            if(!disp_cnt.bg_enable[2])
            {
                break;
            }
            

            for(uint32_t x = 0; x < SCREEN_WIDTH; x++)
            {
                if(bg_window_enabled(2,x))
                {
                    const uint32_t c = convert_color(handle_read<uint16_t>(mem.vram,(ly*SCREEN_WIDTH*2)+x*2));
                    screen[(ly*SCREEN_WIDTH)+x] = c;
                }
            }
            break;
        }


        case 0x4: // mode 4 (does not handle scrolling)
        {
            // bg 2 enable for this
            if(!disp_cnt.bg_enable[2])
            {
                break;
            }
            
            for(uint32_t x = 0; x < SCREEN_WIDTH; x++)
            {
                if(bg_window_enabled(2,x))
                {
                    const uint8_t idx = mem.vram[(ly*SCREEN_WIDTH)+x];
                    const uint16_t color = handle_read<uint16_t>(mem.pal_ram,(idx*2));
                    const uint32_t c = convert_color(color);
                    screen[(ly*SCREEN_WIDTH)+x] = c;
                }
            }
            break;
        }

/*
        case 0x5: // same as mode 3 but lower screen size?
        {
            for(uint32_t x = 0; x < SCREEN_WIDTH; x++)
            {
                uint32_t c = convert_color(handle_read<uint16_t>(mem.vram,(ly*SCREEN_WIDTH*2)+x*2));
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

    merge_layers();
}

}
