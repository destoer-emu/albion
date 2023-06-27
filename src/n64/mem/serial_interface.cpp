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
            unimplemented("si write: %x\n",addr);
            break;
        }
    }
}

u32 read_si(N64& n64, u64 addr)
{
    UNUSED(n64);

    switch(addr)
    {
        default:
        {
            unimplemented("si read: %x\n",addr);
        }
    }
}

}