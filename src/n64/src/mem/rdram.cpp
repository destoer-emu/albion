
namespace nintendo64
{

void write_rdram_regs(N64& n64, u64 addr, u32 v)
{
    // these configure ram we dont really care about strict emulation of them
    // just have the writes go through
    const u32 offset = addr - 0x03F00000;

    if(addr <= 0x03F0'0028)
    {
        n64.mem.rd_ram_regs[offset] = v;
    }
}

u32 read_rdram_regs(N64& n64, u64 addr)
{
    // these configure ram we dont really care about strict emulation of them
    // just have the reads go through
    const u32 offset = addr - 0x03F00000;

    if(addr <= 0x03F0'0028)
    {
        return n64.mem.rd_ram_regs[offset];
    }    

    return 0;
}

void write_ri(N64& n64, u64 addr ,u32 v)
{
    auto& ri = n64.mem.ri;

    switch(addr)
    {
        case RI_SELECT: ri.select = v & 0b111111; break;
        case RI_CONFIG: ri.config = v & 0b1111111; break;
        case RI_CURRENT_LOAD: break; // write only ignore for now
        case RI_BASE: ri.base = v & 0b1111; break;
        case RI_REFRESH: ri.refresh = v & 0b1111111111111111111; break;

        default: break;
    }    
}

u32 read_ri(N64& n64, u64 addr)
{
    auto& ri = n64.mem.ri;

    switch(addr)
    {
        case RI_SELECT: return ri.select;
        case RI_CONFIG: return ri.config;
        case RI_BASE: return ri.base;
        case RI_REFRESH: return ri.refresh;

        default: return 0;
    }    
}

}