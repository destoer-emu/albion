namespace nintendo64
{

void check_mi_interrupts(N64& n64)
{
    if(n64.mem.mi_mask & n64.mem.mi_intr)
    {
        mi_intr(n64);
    }
}


void write_mi(N64& n64, u64 addr, u32 v)
{
    switch(addr)
    {  
        case MI_MODE_REG: 
        {
            n64.mem.mi_mode = v; 
            if(is_set(v,11)) // bit 11 wrote clear DP interrupt bit 
            {
                n64.mem.mi_intr = deset_bit(n64.mem.mi_intr,DP_INTR_BIT);
            }
            break;
        }

        case MI_INTR_MASK_REG:
        {
            Mem &mem = n64.mem;

            mem.mi_mask = deset_bitset_if_set(mem.mi_mask,v,0,SP_INTR_BIT);
            mem.mi_mask = set_bitset_if_set(mem.mi_mask,v,1,SP_INTR_BIT);

            mem.mi_mask = deset_bitset_if_set(mem.mi_mask,v,2,SI_INTR_BIT);
            mem.mi_mask = set_bitset_if_set(mem.mi_mask,v,3,SI_INTR_BIT);

            mem.mi_mask = deset_bitset_if_set(mem.mi_mask,v,4,AI_INTR_BIT);
            mem.mi_mask = set_bitset_if_set(mem.mi_mask,v,5,AI_INTR_BIT);

            mem.mi_mask = deset_bitset_if_set(mem.mi_mask,v,6,VI_INTR_BIT);
            mem.mi_mask = set_bitset_if_set(mem.mi_mask,v,7,VI_INTR_BIT);

            mem.mi_mask = deset_bitset_if_set(mem.mi_mask,v,8,PI_INTR_BIT);
            mem.mi_mask = set_bitset_if_set(mem.mi_mask,v,9,PI_INTR_BIT);

            mem.mi_mask = deset_bitset_if_set(mem.mi_mask,v,10,DP_INTR_BIT);
            mem.mi_mask = set_bitset_if_set(mem.mi_mask,v,11,DP_INTR_BIT);
            check_mi_interrupts(n64);
            break;
        }

        default: unimplemented("write_mem: mips interface : %8x\n",addr); break;
    }
}

void set_mi_interrupt(N64& n64, u32 bit)
{
    n64.mem.mi_intr = set_bit(n64.mem.mi_intr,bit);
    check_mi_interrupts(n64);
}

void deset_mi_interrupt(N64& n64, u32 bit)
{
    n64.mem.mi_intr = deset_bit(n64.mem.mi_intr,bit);
}

}