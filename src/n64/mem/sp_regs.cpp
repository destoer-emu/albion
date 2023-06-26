namespace nintendo64
{
void write_sp_regs(N64& n64, u64 addr ,u32 v)
{
    auto& sp = n64.mem.sp_regs;

    switch(addr)
    {
        case SP_STATUS_REG:
        {
            sp.halt = deset_if_set(sp.halt,v,0);
            sp.halt = set_if_set(sp.halt,v,1);

            sp.broke = deset_if_set(sp.broke,v,2);

            // TODO: should this actually be in the mips int reg
            // check this when we get intr
            sp.intr = deset_if_set(sp.intr,v,3);
            sp.intr = set_if_set(sp.intr,v,4);

            sp.single_step = deset_if_set(sp.single_step,v,5);
            sp.single_step = set_if_set(sp.single_step,v,6);

            sp.clear_intr_on_break = is_set(v,7);
            sp.set_intr_on_break = is_set(v,8);

            // handle signal sets
            for(u32 i = 0; i < 8; i++)
            {
                const u32 idx = ((i + 9) * 2);
                sp.signal[i] = set_if_set(sp.signal[i],idx,v);
            }

            // handle signal clear
            for(u32 i = 0; i < 8; i++)
            {
                const u32 idx = ((i + 10) * 2);
                sp.signal[i] = deset_if_set(sp.signal[i],idx,v);
            }

            break;
        }

        default:
        {
            unimplemented("write_mem: sp regs: %08x : %08x\n",addr,v);
            break;
        }
    }
}

u32 read_sp_regs(N64& n64, u64 addr)
{
    auto& sp = n64.mem.sp_regs;

    switch(addr)
    {
        case SP_PC_REG:
        {
            return sp.pc;
        }
    
        default:
        {
            unimplemented("read_mem: sp regs %8x\n",addr);
            return 0;
        }
    }    
}

}
