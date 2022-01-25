#include <n64/n64.h>

namespace nintendo64
{

void reset_rdp(Rdp &rdp, u32 x, u32 y)
{
    change_res(rdp,x,y);
}

void change_res(Rdp &rdp, u32 x, u32 y)
{
    printf("res change %d : %d : %d\n",x,y, x * y);

    rdp.screen_x = x;
    rdp.screen_y = y;
    rdp.screen.resize(x * y);
    std::fill(rdp.screen.begin(),rdp.screen.end(),0xff000000);
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