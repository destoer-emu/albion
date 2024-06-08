namespace nintendo64
{

void insert_sp_dma_event(N64& n64)
{
    // How long should this take?
    const auto event = n64.scheduler.create_event(20,n64_event::sp_dma);
    n64.scheduler.insert(event,false);    
}

void do_sp_dma(N64& n64, b32 to_rdram)
{
    auto& sp = n64.mem.sp_regs;

    const auto& reg = to_rdram? sp.write_dma : sp.read_dma;

    sp.dma_busy = true;

    u8* sp_mem = sp.dmem_or_imem? n64.mem.sp_dmem.data() : n64.mem.sp_imem.data();

    if(to_rdram)
    {
        u32 dst = sp.dram_addr;
        u32 src = sp.mem_addr; 

        for(u32 c = 0; c < reg.count; c++)
        {
            for(u32 i = 0; i < reg.len; i++)
            {
                n64.mem.rd_ram[(dst + i) & (n64.mem.rd_ram.size() - 1)] = sp_mem[(src + i) & 0xfff];
            }

            src += reg.skip;
            dst += reg.skip;
        }
    }

    else
    {
        u32 dst = sp.dram_addr;
        u32 src = sp.mem_addr; 

        for(u32 c = 0; c < reg.count; c++)
        {
            for(u32 i = 0; i < reg.len; i++)
            {
                sp_mem[(src + i) & 0xfff] = n64.mem.rd_ram[(dst + i) & (n64.mem.rd_ram.size() - 1)];
            }

            src += reg.skip;
            dst += reg.skip;
        }
    }

    // add a end event
    insert_sp_dma_event(n64);
}

void sp_dma_finished(N64& n64)
{
    auto& sp = n64.mem.sp_regs;

    // dma over
    sp.dma_busy = false;

    // handle pending transfer
    if(sp.dma_full)
    {
        sp.dma_full = false;

        // we just have it do this instantly
        do_sp_dma(n64,sp.pending_to_rdram);
    }
}

void write_sp_dma(N64& n64, SpDma& reg, u32 v, b32 to_rdram)
{
    reg.len = v & 0xfff;
    reg.count = (v >> 12) & 0xff;
    reg.skip = (v >> 20) & 0xfff;

    auto& sp = n64.mem.sp_regs;

    if(!sp.dma_busy)
    {
        do_sp_dma(n64,to_rdram);
    }

    else if(!sp.dma_full)
    {
        sp.pending_to_rdram = to_rdram;
        sp.dma_full = true;
    }
}

u32 read_sp_dma(SpDma& reg)
{
    return reg.len | reg.count << 12 | reg.skip << 20;
}

void write_sp_regs(N64& n64, u64 addr ,u32 v)
{
    auto& sp = n64.mem.sp_regs;

    switch(addr)
    {
        case SP_PC: 
        {
            sp.pc = v & 0xfff;
            break;
        }

        case SP_STATUS:
        {
            sp.halt = deset_if_set(sp.halt,v,0);
            sp.halt = set_if_set(sp.halt,v,1);

            sp.broke = deset_if_set(sp.broke,v,2);

            if(is_set(v,3))
            {
                deset_mi_interrupt(n64,SP_INTR_BIT);
            }

            if(is_set(v,4))
            {
                set_mi_interrupt(n64,SP_INTR_BIT);
            }

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

        case SP_MEM_ADDR:
        {
            sp.mem_addr = v & 0xfff;  
            sp.dmem_or_imem = is_set(v,12);
            break;
        }

        case SP_DRAM_ADDR:
        {
            sp.dram_addr = v & 0x00ff'ffff;
            break;
        }

        case SP_WR_LEN: 
        {
            write_sp_dma(n64,sp.write_dma,v,true);
            break;
        }

        case SP_RD_LEN: 
        {
            write_sp_dma(n64,sp.read_dma,v,false);
            break;
        }

        case SP_SEMAPHORE: 
        {
            sp.semaphore = false;
            break;
        }

        // read only
        case SP_DMA_BUSY: case SP_DMA_FULL: 
        {
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
        case SP_PC:
        {
            return sp.pc;
        }

        case SP_MEM_ADDR:
        {
            return sp.mem_addr | sp.dmem_or_imem << 12;
        }

        case SP_DRAM_ADDR:
        {
            return sp.dram_addr;
        }

        case SP_WR_LEN: 
        {
            return read_sp_dma(sp.write_dma);
        }

        case SP_RD_LEN: 
        {
            return read_sp_dma(sp.read_dma);
        }

        case SP_SEMAPHORE: 
        {
            sp.semaphore = true;
            return 0;
        }

        case SP_DMA_BUSY:
        {
            return sp.dma_busy;
        }

        case SP_DMA_FULL: 
        {
            return sp.dma_full;
        }
    
        default:
        {
            unimplemented("read_mem: sp regs %8x\n",addr);
            //return 0;
        }
    }    
}

}
