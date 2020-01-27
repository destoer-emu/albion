#include <gb/ppu.h>


// save states
void Ppu::save_state(std::ofstream &fp)
{
    file_write_vec(fp,screen);
    file_write_var(fp,current_line);
    file_write_var(fp,mode);
    file_write_var(fp,new_vblank);
    file_write_var(fp,signal);
    file_write_var(fp,scanline_counter);
    file_write_var(fp,hblank);
    file_write_var(fp,x_cord);
    file_write_arr(fp,ppu_fifo,sizeof(ppu_fifo));
    file_write_var(fp,pixel_idx);
    file_write_var(fp,ppu_cyc);
    file_write_var(fp,ppu_scyc);
    file_write_var(fp,pixel_count);
    file_write_arr(fp,fetcher_tile,sizeof(fetcher_tile));
    file_write_var(fp,tile_cord);
    file_write_var(fp,tile_ready);
    file_write_arr(fp,objects_priority,sizeof(objects_priority));
    file_write_var(fp,no_sprites);
    file_write_var(fp,sprite_drawn);
    file_write_var(fp,window_start);
    file_write_var(fp,x_scroll_tick);
    file_write_var(fp,scx_cnt);
    file_write_arr(fp,bg_pal,sizeof(bg_pal));
    file_write_arr(fp,sp_pal,sizeof(sp_pal));
    file_write_var(fp,sp_pal_idx);
    file_write_var(fp,bg_pal_idx);
}

void Ppu::load_state(std::ifstream &fp)
{
    file_read_vec(fp,screen);
    file_read_var(fp,current_line);
    file_read_var(fp,mode);
    if(static_cast<int>(mode) > 3 || static_cast<int>(mode) < 0)
    {
        throw std::runtime_error("load_state invalid ppu mode!");
    }
    file_read_var(fp,new_vblank);
    file_read_var(fp,signal);
    file_read_var(fp,scanline_counter);
    file_read_var(fp,hblank);
    file_read_var(fp,x_cord);
    file_read_arr(fp,ppu_fifo,sizeof(ppu_fifo));
    file_read_var(fp,pixel_idx);
    file_read_var(fp,ppu_cyc);
    file_read_var(fp,ppu_scyc);
    file_read_var(fp,pixel_count);
    file_read_arr(fp,fetcher_tile,sizeof(fetcher_tile));
    file_read_var(fp,tile_cord);
    file_read_var(fp,tile_ready);
    file_read_arr(fp,objects_priority,sizeof(objects_priority));
    file_read_var(fp,no_sprites);
    file_read_var(fp,sprite_drawn);
    file_read_var(fp,window_start);
    file_read_var(fp,x_scroll_tick);
    file_read_var(fp,scx_cnt);
    file_read_arr(fp,bg_pal,sizeof(bg_pal));
    file_read_arr(fp,sp_pal,sizeof(sp_pal));
    file_read_var(fp,sp_pal_idx);
    file_read_var(fp,bg_pal_idx);
}