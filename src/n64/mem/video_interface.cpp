namespace nintendo64
{

void write_vi(N64& n64, u64 addr ,u32 v)
{
    auto& vi = n64.mem.vi;

    switch(addr)
    {
        case VI_CONTROL:
        {
            vi.bpp = v & 0b11;
            vi.gamma_dither = is_set(v,2);
            vi.gamma = is_set(v,3);
            vi.divot = is_set(v,4);
            vi.serrate = is_set(v,6);
            vi.aa = (v >> 8) & 0b11;

            if(vi.gamma_dither)
            {
                //unimplemented("dither");
            }

            if(vi.gamma)
            {
                //unimplemented("gamma");
            }

            if(vi.divot)
            {
                //unimplemented("divot");
            }

            if(vi.serrate)
            {
                //unimplemented("serrate");
            }

            break;
        }

        case VI_ORIGIN:
        {
            vi.origin = v & 0x00ff'ffff;
            break;
        }

        case VI_WIDTH:
        {
            vi.width = v & 0xfff;

            // how do we get the res from this
            const auto x_old = n64.rdp.screen_x; 
            const auto y_old = n64.rdp.screen_y;

            // do we assume 4:3?
            change_res(n64,vi.width,(vi.width / 4) * 3);

            n64.size_change = (n64.rdp.screen_x != x_old || n64.rdp.screen_y != y_old);
            break;
        }

        case VI_INTR:
        {
            vi.intr = v & 0x3ff;
            break;
        }

        case VI_BURST:
        {
            vi.burst_start = (v >> 20) & 0x3ff;
            vi.vsync_width = (v >> 16) & 0xf;
            vi.burst_width = (v >> 8) & 0xff;
            vi.hsync_width = (v >> 0) & 0xff;
            break;
        }

        case VI_V_SYNC:
        {
            vi.vsync = v & 0x3ff;
            break;
        }

        case VI_H_SYNC:
        {
            vi.hsync = v & 0x3ff;
            break;
        }     

        // current line when written clears vi intr
        case VI_CURRENT:
        {
            deset_mi_interrupt(n64,VI_INTR_BIT);
            break;
        }

        case VI_LEAP:
        {
            vi.leap_a = (v >> 16) & 0xfff;
            vi.leap_b = v & 0xfff;
            break;
        }

        case VI_H_START:
        {
            vi.h_start = (v >> 16) & 0x3ff;
            vi.h_end = v & 0x3ff;
            break;
        }

        case VI_V_START:
        {
            vi.v_start = (v >> 16) & 0x3ff;
            vi.v_end = v & 0x3ff;
            break;                
        }

        case VI_V_BURST:
        {
            vi.vburst_start = (v >> 16) & 0x3ff;
            vi.vburst_end = v & 0x3ff;
            break;
        }

        case VI_X_SCALE:
        {
            vi.x_offset = (v >> 16) & 0xfff;
            vi.x_scale = v & 0xfff;
            break;
        }

        case VI_Y_SCALE:
        {
            vi.y_offset = (v >> 16) & 0xfff;
            vi.y_scale = v & 0xfff;
            break;
        }

        default: unimplemented("write_mem: video interface: %8x\n",addr); break;
    }    
}

u32 read_vi(N64& n64, u64 addr)
{
    //auto& vi = n64.mem.vi;

    switch(addr)
    {
        case VI_CURRENT: 
        {
            // TODO: what register controls the interlacing behavior?
            return n64.rdp.ly & ~0b1;
        }

        default:
        {
            unimplemented("read_mem: video interface: %8x\n",addr);
            return 0;
        }
    }    
}

}