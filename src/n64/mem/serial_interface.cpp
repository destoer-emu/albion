namespace nintendo64
{

void insert_si_event(N64& n64)
{
    // how long does this take?
    const auto event = n64.scheduler.create_event(16,n64_event::si_dma);
    n64.scheduler.insert(event,false);    
}

void si_dma_finished(N64& n64)
{
    auto& si = n64.mem.si;

    set_mi_interrupt(n64,SI_INTR_BIT);
    si.dma_busy = false;    
}

void do_si_dma(N64& n64, u64 src, u64 dst)
{
    for(u32 i = 0; i < 64; i += 4)
    {
        const u32 data = read_physical<u32>(n64,src + i);
        write_physical<u32>(n64,dst+i,data);
    }

    n64.mem.si.dma_busy = true;

    insert_si_event(n64);    
}

void write_si(N64& n64, u64 addr, u32 v)
{
    auto& si = n64.mem.si;

    switch(addr)
    {
        // writes clear interruptt line for si
        case SI_STATUS:
        {
            deset_mi_interrupt(n64,SI_INTR_BIT);
            break;
        }

        case SI_DRAM_ADDR:
        {
            si.dram_addr = v & 0x00ff'ffff;
            break;
        }

        case SI_PIF_AD_RD64B:
        {
            if(n64.mem.joybus_enabled)
            {
                joybus_comands(n64);
            }

            do_si_dma(n64,v,si.dram_addr);
            break;            
        }

        case SI_PIF_AD_WR64B:
        {
            // new write joybus commands are out
            n64.mem.joybus_enabled = false;

            do_si_dma(n64,si.dram_addr,v);
            break;
        }

        default:
        {
            unimplemented("si write: %x\n",addr);
            break;
        }
    }
}

u32 read_si(N64& n64, u64 addr)
{
    auto& si = n64.mem.si;

    switch(addr)
    {
        case SI_STATUS:
        {
            return (si.dma_busy << 0) | (si.io_busy << 1) | (si.read_pending << 2) |
                (si.dma_error << 3) | (mi_intr_set(n64,SI_INTR_BIT) << 12);
        }

        default:
        {
            unimplemented("si read: %x\n",addr);
        }
    }
}

}