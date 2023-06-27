namespace nintendo64
{
void write_ai(N64& n64, u64 addr ,u32 v)
{
    UNUSED(v);

    switch(addr)
    {
        // writes clear interrupt
        case AI_STATUS:
        {
            deset_mi_interrupt(n64,AI_INTR_BIT);
            break;
        }

        default:
        {
            //unimplemented("ai write: %x\n",addr);
            break;
        }
    }
}

u32 read_ai(N64& n64, u64 addr)
{
    UNUSED(n64);

    switch(addr)
    {

        default:
        {
            //unimplemented("ai read: %x\n",addr);
            return 0;
            break;
        }
    }
}
}