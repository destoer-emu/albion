#include <gba/gba.h>

namespace gameboyadvance
{

/*  TODO
    mosiac,unstub the memory writes
*/

Display::Display(GBA &gba) : mem(gba.mem), cpu(gba.cpu), scheduler(gba.scheduler)
{
    screen.resize(SCREEN_WIDTH*SCREEN_HEIGHT);
    for(auto &x: scanline)
    {
        // two tile lee weigh so we can just draw extra
        // and scroll into it
        x.resize(SCREEN_WIDTH);
    }
    sprite_line.resize(SCREEN_WIDTH);
    sprite_semi_transparent.resize(SCREEN_WIDTH);
    window.resize(SCREEN_WIDTH);
    oam_priority.resize(SCREEN_WIDTH);
    sprite_priority.resize(SCREEN_WIDTH);
}

void Display::init()
{
    std::fill(screen.begin(),screen.end(),0);
    cyc_cnt = 0; // current number of elapsed cycles
    ly = 0;
    mode = display_mode::visible;
    new_vblank = false;
    disp_io.init();

    window_0_y_triggered = false;
    window_1_y_triggered = false;
    insert_new_ppu_event();	
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

    // just easier to handle vblank exit by here
    if(ly == 228)
    {
        // exit vblank
        mode = display_mode::visible;
        ly = 0;
    }

    // not set on line 227
    else if(ly == 227)
    {
        disp_io.disp_stat.vblank = false;
    }


    // see window_midframe.gba
    // when is this checked? in hblank or line start?

    if(ly == disp_io.win0v.y2)
    {
        window_0_y_triggered = false;
    }

    else if(ly == disp_io.win0v.y1)
    {
        window_0_y_triggered = true;
    }



    if(ly == disp_io.win1v.y2)
    {
        window_1_y_triggered = false;
    }


    else if(ly == disp_io.win1v.y1)
    {
        window_1_y_triggered = true;
    }



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
    cyc_cnt -= 1232; // reset cycle counter
}




void Display::tick(int cycles)
{
    cyc_cnt += cycles;

    switch(mode)
    {
        case display_mode::visible:
        {
            // enter hblank 
            if(cyc_cnt >= 1006)
            {


                // if in vdraw render the line
                if(ly < SCREEN_HEIGHT)
                {
                    render();
                }

                mode = display_mode::hblank;
                disp_io.disp_stat.hblank = true;

                // flag should not get set to later

                // if hblank irq enabled
                if(disp_io.disp_stat.hblank_irq_enable)
                {
                    cpu.request_interrupt(interrupt::hblank);
                }
                mem.dma.handle_dma(dma_type::hblank);
            }
            insert_new_ppu_event();	
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
                    new_vblank = true;

                    // if vblank irq enabled
                    if(disp_io.disp_stat.vblank_irq_enable)
                    {
                        cpu.request_interrupt(interrupt::vblank);
                    }
                    mem.dma.handle_dma(dma_type::vblank);

                    // reload internal ref point registers
                    disp_io.bg2_ref_point.int_ref_point_x = disp_io.bg2_ref_point.ref_point_x;
                    disp_io.bg2_ref_point.int_ref_point_y = disp_io.bg2_ref_point.ref_point_y;
     
                    disp_io.bg3_ref_point.int_ref_point_x = disp_io.bg3_ref_point.ref_point_x;
                    disp_io.bg3_ref_point.int_ref_point_y = disp_io.bg3_ref_point.ref_point_y;

                }

                else
                {
                    mode = display_mode::visible;
                }
            }

            insert_new_ppu_event();	
            break;
        }

        case display_mode::vblank:
        {
            // inc a line
            if(cyc_cnt >= 1232)
            {
                advance_line();
            }
            
            else if(cyc_cnt >= 1006) 
            {
                // enter hblank (dont set the internal mode here)
                disp_io.disp_stat.hblank = true;

                if(disp_io.disp_stat.hblank_irq_enable)
                {
                    cpu.request_interrupt(interrupt::hblank);
                }


                // dma does not happen here
            }
            insert_new_ppu_event();	
            break;
        }
    }
}

void Display::insert_new_ppu_event()
{
    uint32_t cycles = 0;

    switch(mode)
    {
        case display_mode::vblank:
        {
            cycles = cyc_cnt < 1006? 1006 - cyc_cnt : 1232 - cyc_cnt;
            break; 
        }

        case display_mode::hblank:
        {
            cycles = 1232 - cyc_cnt;
            break;
        }

        case display_mode::visible:
        {
            cycles = 1006 - cyc_cnt;
            break;
        }
    }

    const auto event = scheduler.create_event(cycles,gba_event::display);
    scheduler.insert(event,false);       
}

}