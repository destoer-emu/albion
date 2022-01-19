#include <n64/n64.h>

namespace nintendo64
{

void reset_rdp(Rdp &rdp, u32 x, u32 y)
{
    printf("res change %d : %d\n",x,y);
    change_res(rdp,x,y);
}

void change_res(Rdp &rdp, u32 x, u32 y)
{
    rdp.screen_x = x;
    rdp.screen_y = y;
    rdp.screen.resize(y * x);
    std::fill(rdp.screen.begin(),rdp.screen.end(),0xff000000);
}

void render(N64 &n64)
{
    // kinda assume some settings for just to get the test output
    UNUSED(n64);

    switch(n64.mem.vi_bpp)
    {
        // rgb 5551
        case 1:
        {
            // TODO: handle alpha
            // this probably has more to it but just a plain copy for now
            for(u32 i = 0; i < n64.rdp.screen.size(); i++)
            {
                //const u32 addr = i * sizeof(u16) + n64.mem.vi_origin;
                // FIXME: hack to fix output 
                const u32 addr = i * sizeof(u32) + n64.mem.vi_origin;
                const auto v = handle_read<u16>(n64.mem.rd_ram,addr);

                if(is_set(v,15))
                {
                    n64.rdp.screen[i] = convert_color(v);
                }
            }
            break;
        }

        // 8bpp
        case 2:
        {
            // this probably has more to it but just a plain copy for now
            for(u32 i = 0; i < n64.rdp.screen.size(); i++)
            {
                const u32 addr = i * sizeof(u32) + n64.mem.vi_origin;
                n64.rdp.screen[i] = handle_read<u32>(n64.mem.rd_ram,addr) | 0xff000000;
            }
            break;
        }

        default: printf("unhandled bpp mode %x\n",n64.mem.vi_bpp); exit(1);
    }


}


}