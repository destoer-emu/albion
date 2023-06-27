namespace nintendo64
{
void write_si(N64& n64, u64 addr, u32 v)
{
    auto& si = n64.mem.si;
    auto& mem = n64.mem;

    UNUSED(si); UNUSED(v);


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
            const u32 addr = si.dram_addr & 0x7fffff;
            memcpy(&mem.rd_ram[addr],&mem.pif_ram[0],64);
            set_mi_interrupt(n64,SI_INTR_BIT);
            break;            
        }

        case SI_PIF_AD_WR64B:
        {
            const u32 addr = si.dram_addr & 0x7fffff;
            memcpy(&mem.pif_ram[0],&mem.rd_ram[addr],64);
            set_mi_interrupt(n64,SI_INTR_BIT);
            
            // we have wrote in a command handle it
            handle_pif_commands(n64);
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