#include <gba/gba.h>

namespace gameboyadvance
{

// for frontend debugging

void Display::render_palette(uint32_t *palette, size_t size)
{
    for(size_t i = 0; i < size; i++)
    {
        palette[i] = convert_color(handle_read<uint16_t>(mem.pal_ram,i*2));
    }   
}


void Display::render_map(int id, std::vector<uint32_t> &map)
{
    assert(map.size() == 512 * 512);

    std::fill(map.begin(),map.end(),0xffffffff);

    uint32_t old_entry = 0xffffffff;

    const auto &cnt = disp_io.bg_cnt[id];
    const uint32_t bg_tile_data_base = cnt.char_base_block * 0x4000;
    const uint32_t size = cnt.screen_size;  


    // 256 color one pal 8bpp? or 16 color 16 pal 4bpp 
    const bool col_256 = cnt.col_256;
        



    const uint32_t y_size_table[] = {256,256,512,512};
    const uint32_t x_size_table[] = {256,512,256,512};

    const auto width = x_size_table[size];
    const auto height = y_size_table[size];

    TileData buf[8];

    for(uint32_t y = 0; y < height; y++)
    {
        // what is the start tiles
        uint32_t map_x = 0; 
        const uint32_t map_y = y / 8;


        // add the current y offset to the base for this line
        // 32 by 32 map so it wraps around again at 32 
        uint32_t bg_map_base = (cnt.screen_base_block * 0x800) + (map_y % 0x20) * 64; // (2 * 32);
                
        for(uint32_t x = 0; x < width; x += 8)
        {
            // 8 for each map but each map takes 2 bytes
            // its 32 by 32 so we want it to wrap back around
            // at that point
            uint32_t bg_map_offset = (map_x++ % 0x20) * 2; 
    

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
                    bg_map_offset += x > 255 ? 0x800 : 0;
                    break;
                }

                case 2: // 256 by 512
                {
                    bg_map_offset += y > 255 ? 0x800 : 0;
                    break;                        
                }

                case 3: // 512 by 512
                {
                    bg_map_offset += y > 255? 0x1000 : 0;
                    bg_map_offset += x > 255? 0x800 : 0;
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
                read_tile(buf,id,col_256,bg_tile_data_base,pal_num,tile_num,y,x_flip,y_flip);
                old_entry = bg_map_entry;
            }

            // read the chunk back into the screen (we should really have something optimised)
            // to pull the colors directly but this will do for now.
            for(int i = 0; i < 8; i++)
            {
                const auto &b = buf[i];

                //printf("%08x\n",color);

                // just use max size. and we will not bother writing off the end
                map[(y * 512) + x + i] = convert_color(b.color);
            }
        }
    }
}


}