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

void Display::read_viewer_tile(TileData tile[],unsigned int bg,bool col_256,uint32_t base,uint32_t pal_num,uint32_t tile_num, 
    uint32_t y,bool x_flip, bool y_flip)
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

            tile[x].color = read_bg_palette(0,tile_data);
            tile[x].source = static_cast<pixel_source>(bg);
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

            tile[x].color = read_bg_palette(pal_num,idx1);
            tile[x].source = static_cast<pixel_source>(bg);

            tile[x+1].color = read_bg_palette(pal_num,idx2);
            tile[x+1].source = static_cast<pixel_source>(bg);
        }
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

            UNUSED(old_entry);
            //if(bg_map_entry != old_entry)
            {

                const bool x_flip = is_set(bg_map_entry,10);
                const bool y_flip = is_set(bg_map_entry,11);

                const uint32_t tile_num = bg_map_entry & 0x3ff; 
                const uint32_t pal_num = (bg_map_entry >> 12) & 0xf;

                // ok our emerald bug symtom is trash written into the first tile slot at 
                // 0x06008000 sometimes... time to trace this

                // render a full tile but then just lie and say we rendered less
                read_viewer_tile(buf,id,col_256,bg_tile_data_base,pal_num,tile_num,y,x_flip,y_flip);
                old_entry = bg_map_entry;
            }

            // read the chunk back into the screen (we should really have something optimised)
            // to pull the colors directly but this will do for now.
            for(int i = 0; i < 8; i++)
            {
                const auto &b = buf[i];

                //printf("%08x\n",color);

                // just use max size. and we will not bother writing off the end
                map[(y * 512) + x + i] = b.color;
            }
        }
    }
}


}