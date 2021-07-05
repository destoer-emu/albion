#include <n64/n64.h>
#include <n64/mem_constants.h>

namespace nintendo64
{

void reset_mem(Mem &mem, const std::string &filename)
{
    // read rom in and hle the pif rom
    read_file(filename,mem.rom);

    // init memory
    // 4mb rd ram
    mem.rd_ram.resize(4 * 1024 * 1024);
    mem.sp_dmem.resize(0x1000);

    const auto magic = handle_read<uint32_t>(mem.rom,0x0);

    // if rom is middle endian byteswap it

    if(magic != 0x80371240)
    {
        puts("byteswapping rom");
        std::iter_swap(mem.rom.begin(),mem.rom.end()-1);
    }

    // hle pif rom
    memcpy(mem.sp_dmem.data(),mem.rom.data(),0x1000);
}

// TODO: this will probably have to be switched over to software page table
// come time for implementing the tlb but lets just keep things nice and simple for now
// and do a giant if else chain for everything
u32 remap_addr(u32 addr)
{
    if(addr < 0x80000000)
    {
        printf("KUSEG %8x\n",addr);
        exit(1);
    }

    // KSEG0 (direct mapped cached)
    else if(addr < 0xa0000000)
    {
        return addr - 0x80000000;
    }

    // KSEG1 (direct mapped uncached)
    else if(addr < 0xc0000000)
    {
        return addr - 0xa0000000;
    }

    // KSSEG
    else if(addr < 0xe0000000)
    {
        printf("KSSEG %8x\n",addr);
        exit(1);
    }

    else
    {
        printf("KSEG3 %8x\n",addr);
        exit(1);
    }

    assert(false);
    return 0;
}


// for now assume accesses are force aligned
// however they are supposed to throw exceptions
// when they are not

template<typename access_type>
access_type read_mem(N64 &n64, u32 addr)
{
    // force align addr
    addr &= ~(sizeof(access_type)-1);   

    UNUSED(n64);
    addr = remap_addr(addr);

    // just do something naive for now so we can get roms running
    if(addr < 0x00800000)
    {
        unimplemented("rdram");
        return 0;
    }

    // UNUSED
    else if(addr < 0x03F00000)
    {
        return 0;
    }

    else if(addr < 0x04000000)
    {
        unimplemented("rdram regs");
        return 0;
    }

    else if(addr < 0x04001000)
    {
        return handle_read<access_type>(n64.mem.sp_dmem,addr & 0xfff);
    }

    // UNUSED
    else if(addr < 0x04040000)
    {
        return 0;
    }

    // TODO: start here impl the read lw is doing
    else if(addr < 0x04100000)
    {
        unimplemented("sp regs");
        return 0;
    }

    else if(addr < 0x04200000)
    {
        unimplemented("dp command regs");
        return 0;
    }

    else if(addr < 0x04300000)
    {
        unimplemented("dp span regs");
        return 0;
    }

    else if(addr < 0x04400000)
    {
        unimplemented("mips interface");
        return 0;
    }

    else if(addr < 0x04500000)
    {
        unimplemented("video interface");
        return 0;
    }

    else if(addr < 0x04600000)
    {
        unimplemented("audio interface");
        return 0;
    }

    else if(addr < 0x04700000)
    {
        unimplemented("peripheral interface");
        return 0;
    }
    
    else if(addr < 0x04800000)
    {
        // UNUSED
        if(addr >= 0x04700020)
        {
            return 0;
        }

        // TODO: should these all be byte accesses?
        switch(addr)
        {
            case RI_SELECT_REG: return n64.mem.ri_select;

            default: unimplemented("rdram interface");
        }

        return 0;
    }

    
  

    else
    {
        printf("unknown physical address: %8x\n",addr);
        exit(1);
        return 0;
    }
}

u8 read_u8(N64 &n64,u32 addr)
{
    return read_mem<u8>(n64,addr);
}

u16 read_u16(N64 &n64,u32 addr)
{
    return bswap(read_mem<u16>(n64,addr));
}

u32 read_u32(N64 &n64,u32 addr)
{
    return bswap(read_mem<u32>(n64,addr));
}

u64 read_u64(N64 &n64,u32 addr)
{
    return bswap(read_mem<u64>(n64,addr));
}



}