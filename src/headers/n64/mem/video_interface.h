namespace nintendo64
{
struct VideoInterface
{
    // vi state
    u32 bpp = 0;
    b8 gamma_dither = 0;
    b8 gamma = 0;
    b8 divot = 0;
    b8 serrate = 0;
    u32 aa = 0;

    u32 origin = 0;
    u32 width = 0;

    u32 intr = 0;
    u32 burst = 0;
    u32 vsync = 0;
    u32 hsync = 0;
    u32 leap = 0;
    u32 hstart = 0;
    u32 vstart = 0;
    u32 vburst = 0;
    u32 xscale = 0;
    u32 yscale = 0;    
};

}