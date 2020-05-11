#include <gb/ppu.h>

namespace gameboy
{

// save states
void Ppu::save_state(std::ofstream &fp)
{
    file_write_vec(fp,screen);
    file_write_var(fp,current_line);
    file_write_var(fp,mode);
    file_write_var(fp,new_vblank);
    file_write_var(fp,signal);
    file_write_var(fp,scanline_counter);
    file_write_var(fp,x_cord);
    file_write_var(fp,bg_fifo);
    file_write_var(fp,obj_fifo);
    file_write_var(fp,fetcher);
    file_write_var(fp,objects);
    file_write_var(fp,tile_cord);
    file_write_var(fp,no_sprites);
    file_write_var(fp,cur_sprite);
    file_write_var(fp,scx_cnt);
    file_write_arr(fp,bg_pal,sizeof(bg_pal));
    file_write_arr(fp,sp_pal,sizeof(sp_pal));
    file_write_var(fp,sp_pal_idx);
    file_write_var(fp,bg_pal_idx);
    file_write_var(fp,window_y_line);
    file_write_var(fp,window_x_line);
    file_write_var(fp,window_drawn);
}

void Ppu::load_state(std::ifstream &fp)
{
    file_read_vec(fp,screen);
    file_read_var(fp,current_line);
    if(current_line > 153)
    {
        throw std::runtime_error("current line out of range!");
    }

    file_read_var(fp,mode);
    if(static_cast<int>(mode) > 3 || static_cast<int>(mode) < 0)
    {
        throw std::runtime_error("load_state invalid ppu mode!");
    }
    file_read_var(fp,new_vblank);
    file_read_var(fp,signal);
    file_read_var(fp,scanline_counter);
    file_read_var(fp,x_cord);

    file_read_var(fp,bg_fifo);
    file_read_var(fp,obj_fifo);
    file_read_var(fp,fetcher);
    file_read_var(fp,objects);

    if(fetcher.len > 8)
    {
        throw std::runtime_error("invalid fecther len");
    }


    if(bg_fifo.read_idx >= bg_fifo.size || bg_fifo.write_idx >= bg_fifo.size || bg_fifo.len > bg_fifo.size)
    {
        throw std::runtime_error("invalid bg fifo");
    } 


    if(obj_fifo.read_idx >= obj_fifo.size || obj_fifo.write_idx >= obj_fifo.size || obj_fifo.len > obj_fifo.size)
    {
        throw std::runtime_error("invalid sp fifo");
    } 

    if(fetcher.len > 8)
    {
        throw std::runtime_error("invalid fetcher len");
    }

    for(size_t i = 0; i < obj_fifo.len; i++)
    {
        size_t fifo_idx = (obj_fifo.read_idx + i) % obj_fifo.len;
        if(obj_fifo.fifo[fifo_idx].sprite_idx >= no_sprites)
        {
            throw std::runtime_error("inavlid sprite index in fifo");
        }
    }


    file_read_var(fp,tile_cord);
    if(tile_cord >= 255) // can fetch past the screen width
    {
        throw std::runtime_error("tile cord out of range!");
    }

    file_read_var(fp,no_sprites);
    file_read_var(fp,cur_sprite);
    if(no_sprites > 10)
    {
        throw std::runtime_error("invalid number of sprites!");
    }

    if(cur_sprite > no_sprites)
    {
        throw std::runtime_error("invalid current sprite");
    }
    file_read_var(fp,scx_cnt);
    file_read_arr(fp,bg_pal,sizeof(bg_pal));
    file_read_arr(fp,sp_pal,sizeof(sp_pal));
    file_read_var(fp,sp_pal_idx);
    file_read_var(fp,bg_pal_idx);
    file_read_var(fp,window_y_line);
    file_read_var(fp,window_x_line);
    file_read_var(fp,window_drawn);
}

}