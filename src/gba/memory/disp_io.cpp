#include <gba/disp_io.h>


RefPoint::RefPoint()
{
    init();
}
    
void RefPoint::init()
{
    ref_point = 0;
}


void RefPoint::write(int idx, uint8_t v)
{
    auto ref_arr = reinterpret_cast<char*>(&ref_point);
    ref_arr[idx] = v;

    // top 4 bits unused
    ref_point &= (0x0fffffff);

    // 27 is sign bit
    if(is_set(ref_point,27))
    {
        ref_point = sign_extend(ref_point,28);
    }
}

BgCnt::BgCnt()
{
    init();
}

void BgCnt::init()
{
    priority = 0;
    char_base_block = 0;
    // 4 - 5 unused(are are zero)
    mosaic = false;
    col_256 = false;
        

    screen_base_block = 0;
    // only used in bg2/bg3
    area_overflow = false;
    screen_size = 0;
}

void BgCnt::write(int idx, uint8_t v)
{
    switch(idx)
    {
        case 0:
        {
            priority = v & 0x3;
            char_base_block = (v >> 2) & 0x3;
            // 4 - 5 unused(are are zero)
            mosaic = is_set(v,6);
            col_256 = is_set(v,7);
            break;
        }

        case 1:
        {
            screen_base_block = v & 31;
            // only used in bg2/bg3
            area_overflow = is_set(v,5);
            screen_size = (v >> 6) & 3;
            break;
        }
    }
}

uint8_t BgCnt::read(int idx) const
{
    switch(idx)
    {
        case 0:
        {
            return priority | char_base_block << 2 |
                mosaic << 6 | col_256 << 7;
        }

        case 1:
        {
            return screen_base_block | area_overflow << 5
                | screen_size << 6;
        }
    }

    // should not be reached
    return 0;
}


BgOffset::BgOffset()
{
    init();
}

void BgOffset::init()
{
    offset = 0;
}

// write only
void BgOffset::write(int idx, uint8_t v)
{
    switch(idx)
    {
        case 0:
        {
            offset &= ~0xff;
            offset |= v;
            break;
        }

        case 1:
        {
            offset &= ~0xff00;
            offset |= (v & 1) << 8;
            break;
        }
    }
}

DispCnt::DispCnt()
{
    init();
}

void DispCnt::init()
{
    bg_mode = 0;
    display_frame = 0;
    hblank_free = false;
    obj_vram_mapping = false;
    forced_blank = false;
    memset(bg_enable,false,sizeof(bg_enable));
    obj_enable = false;
    window0_enable = false;
    window1_enable = false;
    obj_window_enable = false;
}

uint8_t DispCnt::read(int idx) const
{
    switch(idx)
    {
        case 0:
        {
            return bg_mode | display_frame << 4 | hblank_free << 5 |
                obj_vram_mapping << 6 | forced_blank << 7;
            break;
        }

        case 1:
        {
            return bg_enable[0]  | bg_enable[1] << 1 | bg_enable[2] << 2 |
                bg_enable[3] << 3 | obj_enable << 4 | window0_enable << 5 |
                window1_enable << 6 | obj_window_enable << 7;
            break;
        }
    }

    // should not be reached
    return 0;
}

void DispCnt::write(int idx, uint8_t v)
{
    switch(idx)
    {
        case 0:
        {
            bg_mode = v & 7;
            // ignore cgb mode for now
            display_frame = is_set(v,4);
            hblank_free = is_set(v,5);
            obj_vram_mapping = is_set(v,6);
            forced_blank = is_set(v,7);
            break;
        }

        case 1:
        {
            for(int i = 0; i < 4; i++)
            {
                bg_enable[i] = is_set(v,i);
            }

            obj_enable = is_set(v,4);
            window0_enable = is_set(v,5);
            window1_enable = is_set(v,6);
            obj_window_enable = is_set(v,7);
            break;
        }
    }
}


DispStat::DispStat()
{
    init();    
}

void DispStat::init()
{
    vblank = false;
    hblank = false;
    lyc_hit = false;
    vblank_irq_enable = false;
    hblank_irq_enable = false;
    lyc_irq_enable = false;
    lyc = 0;
}


uint8_t DispStat::read(int idx) const
{
    switch(idx)
    {
        case 0:
        {
            return vblank | hblank << 1 | lyc_hit << 2 |
                vblank_irq_enable << 3 | 
                hblank_irq_enable << 4 |
                lyc_irq_enable << 5;
            break;
        }

        case 1:
        {
            return lyc;
            break;
        }
    }

    // should not be reached
    return 0;
}

void DispStat::write(int idx, uint8_t v)
{
    switch(idx)
    {
        case 0:
        {
            vblank = is_set(v,0);
            hblank = is_set(v,1);
            lyc_hit = is_set(v,2);
            vblank_irq_enable = is_set(v,3);
            hblank_irq_enable = is_set(v,4);
            lyc_irq_enable = is_set(v,5);
            break;
        }

        case 1:
        {
            lyc = v;
            break;
        }
    }
}



DispIo::DispIo()
{
    init();
}

void DispIo::init()
{
    // reference points
    bg2x.init();
    bg2y.init();

    // background control
    for(auto &x: bg_cnt)
    {
        x.init();
    }

    for(int i = 0; i < 4; i++)
    {
        bg_offset_x[i].init();
        bg_offset_y[i].init();
    }

    disp_cnt.init();
    disp_stat.init();
}