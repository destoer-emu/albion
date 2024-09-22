#include <n64/n64.h>

namespace nintendo64
{

void insert_line_event(N64 &n64)
{
    const auto event = n64.scheduler.create_event(n64.rdp.line_cycles,n64_event::line_inc);
    n64.scheduler.insert(event,false);     
}


void reset_rdp(N64 &n64)
{
    change_res(n64);

    // for now assume ntsc
    n64.rdp.scan_lines = 525;
    n64.rdp.line_cycles = N64_CLOCK_CYCLES_FRAME / n64.rdp.scan_lines;
    n64.rdp.ly = 0;

    insert_line_event(n64);
}

void change_res(N64 &n64)
{
    auto &rdp = n64.rdp;
    auto &vi = n64.mem.vi;

    u32 x = beyond_all_repair::abs(vi.h_end - vi.h_start);

    // account for half lines
    u32 y = beyond_all_repair::abs(vi.v_end - vi.v_start) >> 1; 
    
    const f32 x_scale = (f32(vi.x_scale) / f32(1 << 10));
    const f32 y_scale = (f32(vi.y_scale) / f32(1 << 10));

    // scale res
    x = (f32(x) * x_scale);
    y = (f32(y + 3) * y_scale);

    spdlog::debug("res change {} : {} : {}\n",x,y, x * y);

    if(x != rdp.screen_x || y != rdp.screen_y)
    {
        n64.size_change = true;
    
        rdp.screen_x = x;
        rdp.screen_y = y;

        rdp.screen.resize(x * y);
        std::fill(rdp.screen.begin(),rdp.screen.end(),0xff000000);
    }
}



void increment_line(N64 &n64)
{
    auto& vi = n64.mem.vi;
    n64.rdp.ly++;

    if((n64.rdp.ly << 1) == vi.intr)
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
    auto& rdp = n64.rdp;

    // kinda assume some settings for just to get the test output
    UNUSED(n64); UNUSED(rdp);


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
            const u32 stride = vi.width;
            const u32 origin = vi.origin;

            //const u32 x_offset = vi.width - rdp.screen_x;

            const u32 x_offset = vi.x_offset >> 10;
            const u32 y_offset = vi.y_offset >> 10;

            for(u32 y = 0; y < rdp.screen_y; y++)
            {
                for(u32 x = 0; x < rdp.screen_x; x++)
                {
                    const u32 offset = ((y + y_offset) * stride) + x + x_offset;
                    
                    const u32 addr = origin + (offset * 2);
                    const auto v = handle_read_n64<u16>(n64.mem.rd_ram,addr);

                    n64.rdp.screen[(y * rdp.screen_x) + x] = convert_color(v);                    
                }
            }
            break;
        }

        // 8bpp
        // what format is this in?
        case 3:
        {
            const u32 stride = vi.width;
            const u32 origin = vi.origin;

            const u32 x_offset = vi.x_offset >> 10;
            const u32 y_offset = vi.y_offset >> 10;

            for(u32 y = 0; y < rdp.screen_y; y++)
            {
                for(u32 x = 0; x < rdp.screen_x; x++)
                {
                    const u32 offset = ((y + y_offset) * stride) + x + x_offset;
                    
                    const u32 addr = origin + (offset * 4);
                    const auto v = handle_read_n64<u32>(n64.mem.rd_ram,addr);

                    // convert to ABGR
                    n64.rdp.screen[(y * rdp.screen_x) + x] = bswap(v);                    
                }
            }

            break;
        }

        default: printf("unhandled bpp mode %x\n",vi.bpp); exit(1);
    }


}


}