namespace nintendo64
{
void write_si(N64& n64, u64 addr, u32 v)
{
    auto& si = n64.mem.si;

    UNUSED(si); UNUSED(v);


    switch(addr)
    {
        // writes clear interruptt line for si
        case SI_STATUS:
        {
            deset_mi_interrupt(n64,SI_INTR_BIT);
            break;
        }

        default:
        {
            //unimplemented("si write: %x\n",addr);
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
            //unimplemented("si read: %x\n",addr);
            return 0;
        }
    }
}

}