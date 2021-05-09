#include <gba/gba.h>

namespace gameboyadvance
{
// TODO
// metroid screen tears (allthough i think this a timing issue)

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


// TODO: specialise this with lambda to only bother drawing 2nd target when we actually need to
// do a blend
void Display::draw_tile(uint32_t x,const TileData &p)
{
    // 1st target is empty
    if(scanline[x].t1.source == pixel_source::bd)
    {
        scanline[x].t1 = p;
    }

    // 2nd target is empty
    else if(scanline[x].t2.source == pixel_source::bd)
    {
        scanline[x].t2 = p;
    }
}

void Display::read_tile(TileData *tile,unsigned int bg,bool col_256,uint32_t base,uint32_t pal_num,uint32_t tile_num, 
    uint32_t y,bool x_flip, bool y_flip)
{
    uint32_t tile_y = y & 7;
    tile_y = y_flip? tile_y ^ 7 : tile_y;


    const TileData DEAD_TILE(read_bg_palette(0,0),pixel_source::bd);

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
            tile[x] = DEAD_TILE;
            if(tile_data)
            {
                tile[x].color = read_bg_palette(0,tile_data);
                tile[x].source = static_cast<pixel_source>(bg);
            }
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
        
        const auto source = static_cast<pixel_source>(bg);
        for(int x = 0; x < 8; x += 2, x_pix += x_step)
        {
            // read out the color indexs from the tile
            const uint8_t tile_data = mem.vram[addr+x_pix];

            const uint32_t idx1 = (tile_data >> shift_one) & 0xf;
            const uint32_t idx2 = (tile_data >> shift_two) & 0xf;

            tile[x] = DEAD_TILE;
            if(idx1)
            {
                tile[x].color = read_bg_palette(pal_num,idx1);
                tile[x].source = source;
            }

            tile[x+1] = DEAD_TILE;
            if(idx2)
            {
                tile[x+1].color = read_bg_palette(pal_num,idx2);
                tile[x+1].source = source;
            }
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

    auto &ref_point = id == 2? disp_io.bg2_ref_point : disp_io.bg3_ref_point;

    // what do i do with actual paramaters here?
    const auto &scale_param = id == 2? disp_io.bg2_scale_param : disp_io.bg3_scale_param;
    
    // unsure how internal ref points work
    auto &ref_point_x = ref_point.int_ref_point_x;
    auto &ref_point_y = ref_point.int_ref_point_y;

    for(uint32_t x = 0; x < SCREEN_WIDTH; x++)
    {
        if(!bg_window_enabled(id,x))
        {
            continue;
        }

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

            // transparent
            else
            {
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

            // transparent
            else
            {
                continue;
            }
        }

        // get tile num from bg map
        const auto tile_num = mem.vram[bg_map_base + ((y_affine / 8) * map_size) + (x_affine / 8)];

        // now figure out where we are offset into the current tile and smash it into the line
        const auto tile_x = x_affine & 7;
        const auto tile_y = y_affine & 7;

        // each tile accounts for 8 vertical pixels but is 64 bytes long
        const uint32_t addr = bg_tile_data_base+(tile_num*0x40) + (tile_y * 8); 
        
        // affine is allways 8bpp
        const uint8_t tile_data = mem.vram[addr+tile_x];
        if(tile_data != 0)
        {
            const auto color = read_bg_palette(0,tile_data);
            draw_tile(x,TileData(color,static_cast<pixel_source>(id)));
        }
    }

    ref_point_x += scale_param.b >> 8;
    ref_point_y += scale_param.d >> 8;
}

// nasty optimisation
bool Display::is_bg_window_trivial(int id)
{
    const auto &win_arr = disp_io.win_cnt.win_arr;
    const auto &disp_cnt = disp_io.disp_cnt;

    // window enabled, window triggered, window has a gap, window has bg disabled.
    // TODO: make this check less verbose
    const bool win0_enable = window_0_y_triggered  && disp_cnt.window0_enable;
    const bool win0_trigger = (win0_enable && disp_io.win0h.x1 != disp_io.win0h.x2 
       && !win_arr[static_cast<size_t>(window_source::zero)].bg_enable[id]);

    const bool win1_enable = window_1_y_triggered  && disp_cnt.window1_enable;
    const bool win1_trigger = (win1_enable && disp_io.win1h.x1 != disp_io.win1h.x2
        && !win_arr[static_cast<size_t>(window_source::one)].bg_enable[id]);

    const bool obj_trigger  = (disp_cnt.obj_window_enable 
        && !win_arr[static_cast<size_t>(window_source::obj)].bg_enable[id]);


    
    const bool bg_window_trivial = !
    (
        win1_trigger || obj_trigger || win0_trigger ||
        (
            // if our bg is not enabled in out our last hope is // one of our enabled windows covers the whole screen
            (disp_cnt.windowing_enabled && !win_arr[static_cast<size_t>(window_source::out)].bg_enable[id]) &&  
        
            !(win0_enable && disp_io.win0h.x1 == 0 && disp_io.win0h.x2 >= SCREEN_WIDTH && win_arr[static_cast<size_t>(window_source::zero)].bg_enable[id]) &&

            !(!win0_trigger && win1_enable && disp_io.win1h.x1 == 0 && 
                disp_io.win1h.x2 >= SCREEN_WIDTH && win_arr[static_cast<size_t>(window_source::one)].bg_enable[id])
        )
    );   

    return bg_window_trivial;
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
    const uint32_t line = (ly + scroll_y) & 511;

    // what is the start tiles
    uint32_t map_x = scroll_x / 8; 
    const uint32_t map_y = line / 8;


    // add the current y offset to the base for this line
    // 32 by 32 map so it wraps around again at 32 
    bg_map_base += (map_y & 0x1f) * 64; // (2 * 32);
            
    uint32_t  pixels_drawn = 0;

    
    // call a function that does not do window checks if it is clear that the entire line has this bg enabled
    const bool bg_window_trivial = is_bg_window_trivial(id);


    TileData tile_data[8];
    for(uint32_t x = 0; x < SCREEN_WIDTH; x += pixels_drawn)
    {
        // 8 for each map but each map takes 2 bytes
        // its 32 by 32 so we want it to wrap back around
        // at that point
        uint32_t bg_map_offset = (map_x++ & 0x1f) * 2; 
   

        

        uint32_t x_pos = (x + scroll_x) & 511;

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

        uint32_t tile_offset;
        if(x == 0)
        {
            tile_offset = x_pos & 7;
            pixels_drawn = 8 - tile_offset;
        }

        else
        {
            tile_offset = 0;
            pixels_drawn = 8;
        }

        if(bg_map_entry != old_entry)
        {

            const bool x_flip = is_set(bg_map_entry,10);
            const bool y_flip = is_set(bg_map_entry,11);

            const uint32_t tile_num = bg_map_entry & 0x3ff; 
            const uint32_t pal_num = (bg_map_entry >> 12) & 0xf;


            // render a full tile but then just lie and say we rendered less
            read_tile(tile_data,id,col_256,bg_tile_data_base,pal_num,tile_num,line,x_flip,y_flip);
            old_entry = bg_map_entry;
        }


        uint32_t end = pixels_drawn;

        // we are going to overdraw we need to clip
        // how much we are drawing
        if(x + end >= SCREEN_WIDTH)
        {
            end = 8 - ((x + end) & 7);  
        }
    
        // we know we are enabled for the entire scanline
        // dont bother to do window checks
        if(bg_window_trivial)
        {
            for(uint32_t i = 0; i < end; i++)
            {
                draw_tile(x+i,tile_data[i+tile_offset]); 
            }
        }

        else
        {
            for(uint32_t i = 0; i < end; i++)
            {
                if(bg_window_enabled(id,x+i))
                {
                    draw_tile(x+i,tile_data[i+tile_offset]);
                }
            }
        }

    }   
 
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
            if(s.source != pixel_source::bd && sprite_window_enabled(x))
            {
                screen[(ly*SCREEN_WIDTH) + x] = convert_color(s.color);
            }
        }
    }

    else
    {

        const auto &bld_cnt = disp_io.bld_cnt;

        // ok so now after we find what exsacly is the first to win
        // we can then check if 1st target
        // and then redo the search starting from it for 2nd target
        // and perform whatever effect if we need to :)
        for(size_t x = 0; x < SCREEN_WIDTH; x++)
        {

            auto &s = sprite_line[x];
            const bool sprite_enable = sprite_window_enabled(x);

            // check color1 prioritys
            // TODO: can we push this off into the sprite rendering code?
            // this will require a pre pass for doing the obj window

            auto &b1 = scanline[x].t1;
            auto &b2 = scanline[x].t2;

            // lower priority is higher, sprite wins even if its equal
            const bool obj_win1 = b1.source == pixel_source::bd || (s.source != pixel_source::bd && sprite_enable &&
                (sprite_priority[x] <= disp_io.bg_cnt[static_cast<uint32_t>(b1.source)].priority) );

            auto &p1 = obj_win1? s : b1;


            // special effects disabled dont care
            if(!special_window_enabled(x))
            {
                screen[(ly*SCREEN_WIDTH) + x] = convert_color(p1.color);
                continue;
            }

            // TODO:
            // if we can trivially see that there wont be any alpha blending on this line
            // dont bother fetching the 2nd color
            
            // check color2 prioritys
            

            // lower priority is higher, sprite wins even if its equal
            // if obj has allready won then we dont care
            const bool obj_win2 = b2.source == pixel_source::bd || (!obj_win1 && s.source != pixel_source::bd && sprite_enable && 
                (sprite_priority[x] <= disp_io.bg_cnt[static_cast<uint32_t>(b2.source)].priority) );

            auto &p2 = obj_win2? s : b2;

            // TODO look at metroid save for edge case with alpha blending
            // handle sfx 

            // if semi transparent object is 1st layer
            // then we need to override the mode to alpha blending
            int special_effect = bld_cnt.special_effect;
            const bool second_target_enable = bld_cnt.second_target_enable[static_cast<int>(p2.source)];
            // if there are overlapping layers and sprite is semi transparent
            // do alpha blend
            const bool semi_transparent = sprite_semi_transparent[x] && p1.source == pixel_source::obj
                && second_target_enable;

            const bool first_target_enable = bld_cnt.first_target_enable[static_cast<int>(p1.source)];

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
                    
                    if((first_target_enable || semi_transparent) && p1.source != pixel_source::bd)
                    {
                        // we have a 1st and second target now we just need to blend them :P
                        if(second_target_enable)
                        {
                            p1.color = do_blend(disp_io.eva,disp_io.evb,p1.color,p2.color);
                        }
                    }
                    break;
                }


                // brighness increase 
                case 2:
                {
                    if(first_target_enable)
                    {
                        p1.color = do_brighten(disp_io.evy,p1.color);
                    }
                    break;
                }

                // brightness decrease
                case 3:
                {
                    if(first_target_enable)
                    {
                        p1.color = do_darken(disp_io.evy,p1.color);
                    }
                    break;
                }
            }

            screen[(ly*SCREEN_WIDTH) + x] = convert_color(p1.color);
            
        }
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

    // figure out which is enabled at current cords
    // if both are prefer win 0

    // first check win0
    // if not enabled check win 1

    std::fill(window.begin(),window.end(), window_source::out);


    if(trigger_0)
    {
        const bool win0_wrap = disp_io.win0h.x1 > disp_io.win0h.x2;

        const uint32_t end = win0_wrap? SCREEN_WIDTH : disp_io.win0h.x2;

        for(uint32_t x = disp_io.win0h.x1; x < end; x++)
        {
            window[x] = window_source::zero;
        }

        if(win0_wrap)
        {
            for(uint32_t x = 0; x < disp_io.win0h.x2; x++)
            {
                window[x] = window_source::zero;
            }
        }
    }


    if(trigger_1)
    {
        const bool win1_wrap = disp_io.win1h.x1 > disp_io.win1h.x2;

        const uint32_t end = win1_wrap? SCREEN_WIDTH : disp_io.win1h.x2;

        for(uint32_t x = disp_io.win1h.x1; x < end; x++)
        {
            if(window[x] != window_source::zero)
            {
                window[x] = window_source::one;
            }
        }

        if(win1_wrap)
        {
            for(uint32_t x = 0; x < disp_io.win1h.x2; x++)
            {
                if(window[x] != window_source::zero)
                {
                    window[x] = window_source::one;
                }
            }
        }
    }    
}


bool Display::bg_window_enabled(unsigned int bg, unsigned int x) const
{
    const auto &disp_cnt = disp_io.disp_cnt;

    // if no windows are active then out of window is not enabled
    if(!disp_cnt.windowing_enabled)
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
    if(!disp_cnt.windowing_enabled)
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
    if(!disp_cnt.windowing_enabled)
    {
        return true;
    }

    return disp_io.win_cnt.win_arr[static_cast<size_t>(window[x])].special_enable;
}


// sort bg so we draw the one with the highest priority into our scanline first
struct BgPriority
{
    int bg;
    int priority;
};

int get_lim(BgPriority *bg_priority, DispIo &disp_io)
{
    const auto disp_cnt = disp_io.disp_cnt;
    const auto render_mode = disp_cnt.bg_mode; 

    // okay what order do we need to render in?
    static constexpr unsigned int bg_limits[3][2] = 
    {
        {0,4},
        {0,3},
        {2,4}
    };

    // ideally id find a nicer way to split off is_bitmap so this is not required
    const auto start = bg_limits[render_mode][0];
    const auto end = bg_limits[render_mode][1];

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

    return lim;  
}

void Display::render()
{
    const auto render_mode = disp_io.disp_cnt.bg_mode; 

    static const Scanline DEAD_PIXEL;
    std::fill(scanline.begin(),scanline.end(),DEAD_PIXEL);

    // ideally we would try to cull draws
    // that are not enabled in the window
    cache_window();
    render_sprites(render_mode);

    switch(render_mode)
    {

        case 0x0: // text mode
        {   
            BgPriority bg_priority[4];
            int lim = get_lim(bg_priority,disp_io);
            //render_bg(0,4);
            for(int i = lim-1; i >= 0; i--)
            {
                render_text(bg_priority[i].bg);   
            }
            break;
        }


        // needs checking
        case 0x1: // text mode
        {
            BgPriority bg_priority[4];
            int lim = get_lim(bg_priority,disp_io);
            // render_bg(0,4)
            // 2 is affine
            for(int i = lim-1; i >= 0; i--)
            {
                const auto bg = bg_priority[i].bg;
                if(bg != 2)
                {
                    render_text(bg);
                }

                else
                {
                    render_affine(bg);
                }   
            }

            break;
        }


        // needs checking
        case 0x2: // bg mode 2
        {
            BgPriority bg_priority[4];
            int lim = get_lim(bg_priority,disp_io);
            //render_bg(2,4);
            for(int i = lim-1; i >= 0; i--)
            {
                const auto bg = bg_priority[i].bg;
                render_affine(bg);  
            }
            break;
        }


        case 0x3: // bg mode 3 
        { 
            // bg 2 enable for this
            if(!disp_io.disp_cnt.bg_enable[2])
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
            if(!disp_io.disp_cnt.bg_enable[2])
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
