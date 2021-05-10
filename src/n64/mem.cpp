#include <n64/n64.h>

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
u64 remap_addr(u64 addr)
{
    printf("%16zx\n",addr);
    if(addr < 0x80000000)
    {
        puts("KUSEG");
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
        puts("KSSEG");
        exit(1);
    }

    else
    {
        puts("KSEG3");
        exit(1);
    }

    assert(false);
    return 0;
}

template<typename access_type>
access_type read_mem(N64 &n64, u64 addr)
{
    UNUSED(n64);
    addr = remap_addr(addr);

    // just do something naive for now so we can get roms running
    if(addr < 0x00800000)
    {
        puts("rdram");
        exit(1);
    }

    // UNUSED
    else if(addr < 0x03F00000)
    {
        return 0;
    }

    else if(addr < 0x04000000)
    {
        puts("rdram regs");
        exit(1);
    }

    else if(addr < 0x04001000)
    {
        // how does aligment work on mips?
        return handle_read<access_type>(n64.mem.sp_dmem,addr & 0xfff);
    }


    else
    {
        printf("unknown address: %16zx",addr);
        exit(1);
        return 0;
    }
}

u8 read_u8(N64 &n64,u64 addr)
{
    return read_mem<u8>(n64,addr);
}

u16 read_u16(N64 &n64,u64 addr)
{
    return bswap(read_mem<u16>(n64,addr));
}

u32 read_u32(N64 &n64,u64 addr)
{
    return bswap(read_mem<u32>(n64,addr));
}

u64 read_u64(N64 &n64,u64 addr)
{
    return bswap(read_mem<u64>(n64,addr));
}



}