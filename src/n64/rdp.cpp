#include <n64/n64.h>

namespace nintendo64
{

void insert_line_event(N64 &n64)
{
    const auto event = n64.scheduler.create_event(n64.rdp.line_cycles,n64_event::line_inc);
    n64.scheduler.insert(event,false);     
}


void reset_rdp(N64 &n64, u32 x, u32 y)
{
    change_res(n64,x,y);
}

void change_res(N64 &n64, u32 x, u32 y)
{
    auto &rdp = n64.rdp;
    printf("res change %d : %d : %d\n",x,y, x * y);

    rdp.screen_x = x;
    rdp.screen_y = y;

    rdp.line_cycles = (N64_CLOCK_CYCLES_FRAME / rdp.screen_y);
    insert_line_event(n64); 

    rdp.screen.resize(x * y);
    std::fill(rdp.screen.begin(),rdp.screen.end(),0xff000000);
}



void increment_line(N64 &n64)
{
    n64.rdp.ly++;

    if(n64.rdp.ly == n64.rdp.screen_y)
    {
        n64.rdp.ly = 0;
        n64.rdp.frame_done = true;
    }

    insert_line_event(n64);
}


void render(N64 &n64)
{
    // kinda assume some settings for just to get the test output
    UNUSED(n64);

    switch(n64.mem.vi_bpp)
    {
        // blank
        case 0:
        {
            std::fill(n64.rdp.screen.begin(),n64.rdp.screen.end(),0xff000000);
            break;
        }

        // rgb 5551
        case 2:
        {
            // TODO: handle alpha
            // this probably has more to it but just a plain copy for now
            for(u32 i = 0; i < n64.rdp.screen.size(); i++)
            {
                const u32 addr = n64.mem.vi_origin + (i * sizeof(u16));
                const auto v = handle_read_n64<u16>(n64.mem.rd_ram,addr);

                if(is_set(v,15))
                {
                    n64.rdp.screen[i] = convert_color(v);
                }
            }
            break;
        }

        // 8bpp
        // what format is this in?
        case 3:
        {
            // this probably has more to it but just a plain copy for now
            for(u32 i = 0; i < n64.rdp.screen.size(); i++)
            {
                const u32 addr = n64.mem.vi_origin + (i * sizeof(u32));

                // TODO: handle alpha properly

                const auto v = handle_read_n64<u32>(n64.mem.rd_ram,addr);

                if(v & 0xff000000)  
                {
                    n64.rdp.screen[i] = v | 0xff000000;
                }
            }
            break;
        }

        default: printf("unhandled bpp mode %x\n",n64.mem.vi_bpp); exit(1);
    }


}


}