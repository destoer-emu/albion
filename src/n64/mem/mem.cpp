#include <n64/n64.h>

namespace nintendo64
{

inline u64 swap_word(u64 v)
{
    return ((v & 0xffffffff) << 32)  | (v >> 32);
}

template<typename access_type>
access_type handle_read_n64(std::vector<u8> &buf, u32 addr)
{
    // handle endianess, we have swapped the entire rom
    // so offset the addresses
    if constexpr(sizeof(access_type) == 2)
    {
        addr ^= 2;
    }

    else if constexpr(sizeof(access_type) == 1)
    {
        addr ^= 3;
    }


    auto v = handle_read<access_type>(buf,addr);

    // 8 byte so we have to swap them
    if constexpr(sizeof(access_type) == 8)
    {
        v = swap_word(v);
    }


    return v;
}

template<typename access_type>
void handle_write_n64(std::vector<u8> &buf, u32 addr, access_type v)
{
    // handle endianess, we have swapped the entire rom
    // so offset the addresses
    if constexpr(sizeof(access_type) == 2)
    {
        addr ^= 2;
    }

    else if constexpr(sizeof(access_type) == 1)
    {
        addr ^= 3;
    }

    // 8 byte so swap words
    if constexpr(sizeof(access_type) == 8)
    {
        v = swap_word(v);
    }


    handle_write<access_type>(buf,addr,v);
}    

void do_pi_dma(N64 &n64, u32 src, u32 dst, u32 len);


void reset_mem(Mem &mem, const std::string &filename)
{
    // read rom in and hle the pif rom
    read_bin(filename,mem.rom);

    if(mem.rom.size() < 32 * 1024 * 1024)
    {
        // ensure rom is power of two!!
        mem.rom.resize(32 * 1024 * 1024);
    }

    else
    {
        unimplemented("large rom");
    }



    // init memory
    // 8mb rd ram
    mem.rd_ram.resize(8 * 1024 * 1024);
    mem.sp_dmem.resize(0x1000);

    mem.sp_imem.resize(0x1000);

    mem.pif_ram.resize(PIF_SIZE);

    auto magic = handle_read<u32>(mem.rom,0x0);

    printf("rom magic: %x\n",magic);

    // if rom is middle endian byteswap it
    if(magic == 0x12408037)
    {
        puts("middle swap rom");
        
        for(u32 i = 0; i < mem.rom.size() - 1; i += 2)
        {
            std::swap(mem.rom[i],mem.rom[i+1]);
        }

    }
    // re read the magic
    magic = handle_read<u32>(mem.rom,0x0);

    printf("rom magic: %x\n",magic);

    // big endian swap around
    if(magic == 0x40123780)
    {
        puts("swapping big endian into le");

        for(u32 i = 0; i < mem.rom.size(); i += sizeof(u32))
        {
            u32 v = handle_read<u32>(mem.rom,i);
            v = bswap(v);
            handle_write<u32>(mem.rom,i,v);
        }
    }


    // hle pif rom
    memcpy(mem.sp_dmem.data(),mem.rom.data(),0x1000);

    mem.ri = {};
    mem.pi = {};
    mem.mi = {};
    mem.vi = {};
    mem.sp_regs = {};
    mem.si = {};
    mem.ai = {};
}

u32 remap_addr(N64& n64,u32 addr)
{
    // TODO: do we care about caching?
    UNUSED(n64);


    const u16 tlb_set = 0b11'11'00'00'11111111;
    const u32 idx = (addr & 0xf000'0000) >> 28;

    if(is_set(tlb_set,idx))
    {
        //return addr & 0x1FFF'FFFF;
        assert(false);
    }

    else
    {
        // NOTE: this only works because both direct mapped sections 
        // are the same size...
        return addr & 0x1FFF'FFFF;
    }
}


// for now assume accesses are force aligned
// however they are supposed to throw exceptions
// when they are not

template<const b32 debug,typename access_type>
void write_mem(N64 &n64, u32 addr, access_type v)
{
    // force align addr
    addr &= ~(sizeof(access_type)-1);   

    addr = remap_addr(n64,addr);

    write_physical<access_type>(n64,addr,v);
}


// for now assume accesses are force aligned
// however they are supposed to throw exceptions
// when they are not

template<const b32 debug,typename access_type>
access_type read_mem(N64 &n64, u32 addr)
{
    // force align addr
    addr &= ~(sizeof(access_type)-1);   

    addr = remap_addr(n64,addr);

    return read_physical<access_type>(n64,addr);
}


template<const b32 debug>
u8 read_u8(N64 &n64,u32 addr)
{
    return read_mem<debug,u8>(n64,addr);
}

template<const b32 debug>
u16 read_u16(N64 &n64,u32 addr)
{
    return read_mem<debug,u16>(n64,addr);
}

template<const b32 debug>
u32 read_u32(N64 &n64,u32 addr)
{
    return read_mem<debug,u32>(n64,addr);
}

template<const b32 debug>
u64 read_u64(N64 &n64,u32 addr)
{
    return read_mem<debug,u64>(n64,addr);
}

template<const b32 debug>
void write_u8(N64 &n64,u32 addr,u8 v)
{
    write_mem<debug,u8>(n64,addr,v);
}

template<const b32 debug>
void write_u16(N64 &n64,u32 addr,u16 v)
{
    write_mem<debug,u16>(n64,addr,v);
}

template<const b32 debug>
void write_u32(N64 &n64,u32 addr,u32 v)
{
    write_mem<debug,u32>(n64,addr,v);
}

template<const b32 debug>
void write_u64(N64 &n64,u32 addr,u64 v)
{
    write_mem<debug,u64>(n64,addr,v);
}



}

#include <n64/mem/mips_interface.cpp>
#include <n64/mem/rdram.cpp>
#include <n64/mem/sp_regs.cpp>
#include <n64/mem/video_interface.cpp>
#include <n64/mem/peripheral_interface.cpp>
#include <n64/mem/pif.cpp>
#include <n64/mem/serial_interface.cpp>
#include <n64/mem/audio_interface.cpp>