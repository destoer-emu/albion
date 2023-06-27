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
            vi.origin = v & 0x00ffffff;
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
            vi.intr = v & 0x7ff;
            break;
        }
        
        // TODO: find out what the hell this thing does lol
        // need to start looking at how rendering works?
        case VI_BURST:
        {
            vi.burst = v;
            break;
        }

        case VI_V_SYNC:
        {
            vi.vsync = v & 0x7ff;
            break;
        }

        case VI_H_SYNC:
        {
            vi.hsync = v & 0x7ff;
            break;
        }     

        // current line when written clears vi intr
        case VI_CURRENT:
        {
            auto& mi = n64.mem.mi;
            mi.intr = deset_bit(mi.intr,VI_INTR_BIT);
            break;
        }

        // TODO: seperate these into there fields
        case VI_LEAP:
        {
            vi.leap = v & 0x0fffffff;
            break;
        }

        case VI_H_START:
        {
            vi.hstart = v & ~0b000000;
            break;
        }

        case VI_V_START:
        {
            vi.vstart = v & ~0b000000;
            break;                
        }

        case VI_V_BURST:
        {
            vi.vburst = v & ~0b000000;
            break;
        }

        case VI_X_SCALE:
        {
            vi.xscale = v & 0x0fffffff;
            break;
        }

        case VI_Y_SCALE:
        {
            vi.yscale = v & 0x0fffffff;
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