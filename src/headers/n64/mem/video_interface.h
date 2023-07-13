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

    u32 intr = 0x3ff;
    
    u32 burst_start = 0;
    u32 burst_width = 0;
    u32 vsync_width = 0;
    u32 hsync_width = 0;

    u32 vsync = 0;
    u32 hsync = 0;

    u32 leap_a = 0;
    u32 leap_b = 0;

    u32 h_start = 0;
    u32 h_end = 0;

    u32 v_start = 0;
    u32 v_end = 0;

    u32 vburst_start = 0;
    u32 vburst_end = 0;

    u32 x_offset = 0;
    u32 x_scale = 0;

    u32 y_offset = 0;  
    u32 y_scale = 0;  


};

}