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

    // for now assume ntsc
    n64.rdp.scan_lines = 525;
    n64.rdp.line_cycles = N64_CLOCK_CYCLES_FRAME / n64.rdp.scan_lines;
    n64.rdp.ly = 0;

    insert_line_event(n64);
}

void change_res(N64 &n64, u32 x, u32 y)
{
    auto &rdp = n64.rdp;
    //printf("res change %d : %d : %d\n",x,y, x * y);

    rdp.screen_x = x;
    rdp.screen_y = y;

    rdp.screen.resize(x * y);
    std::fill(rdp.screen.begin(),rdp.screen.end(),0xff000000);
}



void increment_line(N64 &n64)
{
    auto& vi = n64.mem.vi;
    n64.rdp.ly++;

    if(n64.rdp.ly == vi.intr)
    {
        set_mi_interrupt(n64,VI_INTR_BIT);
    }

    if(n64.rdp.ly == n64.rdp.scan_lines)
    {
        n64.rdp.ly = 0;
        n64.rdp.frame_done = true;
    }

    insert_line_event(n64);
}

/* TODO: understand how this works ourselves
u32 blend(const u32 v1, const u32 v2)
{

}
*/


using ColorLut = std::array<u32,65535>;
constexpr ColorLut pop_color_lut()
{
	ColorLut lut{};

	for(u16 c = 0; c < lut.size(); c++)
	{
        // rgba 5551
    /*
        
        r    g   g   b   a
        <5 | 3>, <2 | 5 | 1> 

        
    */

    /*
        // bswap          
        g    b   a   r   g
        <2 | 5 | 1>, <5 | 3>
    */

    /*
        5   3   2   5   1
        r | g | g | b | a 
    */

		u32 R = (c >> 11) & 0x1f;
		u32 G = (((c >> 8) & 0b111) << 2) | (((c >> 6) & 0b11) << 0);
		u32 B = (c >> 1) & 0x1f;
        const b32 A = (c >> 0) & 0x1;

        R |= R << 3;
        B |= B << 3;
        G |= G << 3;

		// default to standard colors until we add proper correction
		lut[c] =  B << 16 |  G << 8 | R << 0;

        if(A)
        {
            lut[c] |= 0xff00'0000;
        }
	}

	return lut;
}

static constexpr ColorLut COL_LUT = pop_color_lut();

inline u32 convert_color(u16 color)
{
	return COL_LUT[color];
}

void render(N64 &n64)
{
    auto& vi = n64.mem.vi;

    // kinda assume some settings for just to get the test output
    UNUSED(n64);


    switch(vi.bpp)
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
                const u32 addr = vi.origin + (i * sizeof(u16));
                const auto v = handle_read_n64<u16>(n64.mem.rd_ram,addr);

                
                n64.rdp.screen[i] = convert_color(v);
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
                const u32 addr = vi.origin + (i * sizeof(u32));

                const u32 v = handle_read_n64<u32>(n64.mem.rd_ram,addr);

                // convert to ABGR
                n64.rdp.screen[i] = bswap(v); 
            }
            break;
        }

        default: printf("unhandled bpp mode %x\n",vi.bpp); exit(1);
    }


}


}