namespace nintendo64
{

// TODO: handle dma domains properly
void do_pi_dma(N64 &n64, u32 src, u32 dst, u32 len)
{
    printf("dma from %08x to %08x len %08x\n",src,dst,len);

    // for now just do it naviely with a read and write
    // and optimise it with memcpy later
    // len aligned to 16 bit
    for(u32 i = 0; i < len; i += 2)
    {
        const u16 v = read_physical<u16>(n64,src + i);
        write_physical<u16>(n64,dst+i,v);
    }

    // dma is done set the intr flag
    n64.mem.mi.intr = set_bit(n64.mem.mi.intr,PI_INTR_BIT);
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
            deset_mi_interrupt(n64,PI_INTR_BIT);
            break;
        }

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
        case PI_STATUS: return pi.status;

        
        default:
        {
            unimplemented("read_mem: peripheral interface: %8x\n",addr);
            return 0;
        }
    }    
}

}