namespace nintendo64
{

void pi_dma_finished(N64& n64)
{
    auto& pi = n64.mem.pi;
    pi.busy = false;

    // dma is done set the intr flag
    set_mi_interrupt(n64,PI_INTR_BIT);
}

// TODO: handle dma domains properly
void do_pi_dma(N64 &n64, u32 src, u32 dst, u32 len)
{
    printf("dma from %08x to %08x len %08x\n",src,dst,len);

    auto& pi = n64.mem.pi;
    
    pi.busy = true;

    // for now just do it naviely with a read and write
    // and optimise it with memcpy later
    // len aligned to 16 bit
    for(u32 i = 0; i < len; i += 2)
    {
        const u16 v = read_physical<u16>(n64,src + i);
        write_physical<u16>(n64,dst+i,v);
    }


    // TODO: what should the timings on this be?
    const auto event = n64.scheduler.create_event(20,n64_event::pi_dma);
    n64.scheduler.insert(event,false);  
}

void write_pi(N64& n64, u64 addr, u32 v)
{
    auto& pi = n64.mem.pi;

    switch(addr)
    {
        case PI_CART_ADDR:
        {
            // aligned on 2 bytes
            pi.cart_addr = v;
            break;
        }

        case PI_CART_DRAM_ADDR:
        {
            // aligned on 8 bytes
            pi.dram_addr = v & 0xffffff;
            break;
        }

        // need to find proper pi dma info
        // is this where we start a dma?
        case PI_WR_LEN:
        {
            pi.wr_len = v & 0xffffff;
            
            do_pi_dma(n64,pi.cart_addr,pi.dram_addr,pi.wr_len + 1);
            break;
        }

        // clear intr line
        // NOTE: this should also technically stop dmas but ours complete instantly...
        case PI_STATUS:
        {
            if(is_set(v,1))
            {
                deset_mi_interrupt(n64,PI_INTR_BIT);
            }

            // cancel dma
            if(is_set(v,0))
            {
                pi.busy = false;
                n64.scheduler.remove(n64_event::pi_dma,false);
            }

            break;
        }

        case PI_BSD_DOM1_LAT: pi.bsd_dom1_lat = v & 0xff; break;
        case PI_BSD_DOM1_PWD: pi.bsd_dom1_pwd = v & 0xff; break;
        case PI_BSD_DOM1_PGS: pi.bsd_dom1_pgs = v & 0xf; break;
        case PI_BSD_DOM1_RLS: pi.bsd_dom1_rls = v & 0b11; break;

        case PI_BSD_DOM2_LAT: pi.bsd_dom2_lat = v & 0xff; break;
        case PI_BSD_DOM2_PWD: pi.bsd_dom2_pwd = v & 0xff; break;
        case PI_BSD_DOM2_PGS: pi.bsd_dom2_pgs = v & 0xf; break;
        case PI_BSD_DOM2_RLS: pi.bsd_dom2_rls = v & 0b11; break;

        default: 
        {
            unimplemented("write_mem: pi interface: %08x\n",addr);
            break;
        }
    }
}

u32 read_pi(N64& n64, u64 addr)
{
    auto& pi = n64.mem.pi;

    switch(addr)
    {
        case PI_STATUS:
        {
            return (pi.busy << 0) | (mi_intr_set(n64,PI_INTR_BIT) << 3);
        }

        case PI_BSD_DOM1_LAT: return pi.bsd_dom1_lat;
        case PI_BSD_DOM1_PWD: return pi.bsd_dom1_pwd;
        case PI_BSD_DOM1_PGS: return pi.bsd_dom1_pgs;
        case PI_BSD_DOM1_RLS: return pi.bsd_dom1_rls;

        case PI_BSD_DOM2_LAT: return pi.bsd_dom2_lat;
        case PI_BSD_DOM2_PWD: return pi.bsd_dom2_pwd;
        case PI_BSD_DOM2_PGS: return pi.bsd_dom2_pgs;
        case PI_BSD_DOM2_RLS: return pi.bsd_dom2_rls;

        default:
        {
            unimplemented("read_mem: peripheral interface: %8x\n",addr);
            return 0;
        }
    }    
}

}