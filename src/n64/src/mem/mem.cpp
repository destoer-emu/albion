#include <n64/n64.h>
#include "spdlog/spdlog.h"

namespace nintendo64
{

inline u64 swap_word(u64 v)
{
    return ((v & 0xffffffff) << 32)  | (v >> 32);
}

template<typename access_type>
access_type handle_read_n64(const u8* buf, u32 addr)
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

    auto v = handle_read<access_type>(&buf[addr]);

    // 8 byte so we have to swap them
    if constexpr(sizeof(access_type) == 8)
    {
        v = swap_word(v);
    }


    return v;
}

template<typename access_type>
access_type handle_read_n64(std::vector<u8> &buf, u32 addr)
{
    return handle_read_n64<access_type>(buf.data(),addr);
}

template<typename access_type>
void handle_write_n64(u8* buf, u32 addr, access_type v)
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


    handle_write<access_type>(&buf[addr],v);
}  

template<typename access_type>
void handle_write_n64(std::vector<u8> &buf, u32 addr, access_type v)
{
    handle_write_n64(buf.data(),addr,v);
}    

void do_pi_dma(N64 &n64, u32 src, u32 dst, u32 len);


void write_physical_table(Mem& mem,const u32 offset)
{
    const u32 RDRAM_SIZE = 0x0080'0000 / PAGE_SIZE;
    for(u32 i = 0; i < RDRAM_SIZE; i++)
    {
        mem.page_table_read[offset + i] = &mem.rd_ram[i * PAGE_SIZE];
        mem.page_table_write[offset + i] = &mem.rd_ram[i * PAGE_SIZE];
    } 

    const u32 ROM_OFFSET = (0x1000'0000 / PAGE_SIZE);
    const u32 ROM_SIZE = (0x1FC0'0000 - 0x1000'0000) / PAGE_SIZE;

    for(u32 i = 0; i < ROM_SIZE; i++)
    {
        mem.page_table_read[ROM_OFFSET + offset + i] = &mem.rom[i * PAGE_SIZE];
    }
}

void reset_mem(Mem &mem, const std::string &filename)
{
    // read rom in and hle the pif rom
    if(read_bin(filename,mem.rom))
    {
        const auto err = fmt::format("could not open file: {}\n",filename);

        throw std::runtime_error(err);         
    }

    spdlog::info("ROM file " + filename + " read, " + std::to_string(mem.rom.size()) + " bytes.");

    if(mem.rom.size() <= 40 * 1024 * 1024)
    {
        // ensure rom is power of two!!
        mem.rom.resize(40 * 1024 * 1024);
    }

    else
    {
        unimplemented("large rom");
    }



    mem.is_viewer.resize(0x208);

    // init memory
    // 8mb rd ram
    mem.rd_ram.resize(8 * 1024 * 1024);
    memset(&mem.rd_ram[0],0,mem.rd_ram.size());

    mem.sp_dmem.resize(0x1000);

    mem.sp_imem.resize(0x1000);

    // times two so we can be a little lazy with bounds checking...
    mem.pif_ram.resize(PIF_SIZE * 2);


    auto magic = handle_read<u32>(mem.rom,0x0);

    spdlog::debug("ROM Magic Number: " + std::to_string(magic));

    // if rom is middle endian byteswap it
    if(magic == 0x12408037)
    {
        spdlog::debug("Swapping middle-endian ROM..");
        
        for(u32 i = 0; i < mem.rom.size() - 1; i += 2)
        {
            std::swap(mem.rom[i],mem.rom[i+1]);
        }

        // re read the magic
        magic = handle_read<u32>(mem.rom,0x0);

        spdlog::debug("ROM Magic Number: " + std::to_string(magic));
    }

    // big endian swap around
    if(magic == 0x40123780)
    {
        spdlog::debug("Swapping big-endian ROM..");

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
    mem.joybus.enabled = false;

    // setup the page table
    mem.page_table_read.resize(PAGE_TABLE_SIZE);
    mem.page_table_write.resize(PAGE_TABLE_SIZE);

    for(u32 i = 0; i < PAGE_TABLE_SIZE; i++)
    {
        mem.page_table_read[i] = nullptr;
        mem.page_table_write[i] = nullptr;
    }


    write_physical_table(mem,0x8000'0000 / PAGE_SIZE);
    write_physical_table(mem,0xA000'0000 / PAGE_SIZE);
}

u32 remap_addr(N64& n64,u32 addr)
{
    // TODO: do we care about caching?
    UNUSED(n64);


    const u16 tlb_set = 0b11'11'00'00'11111111;
    const u32 idx = (addr & 0xf000'0000) >> 28;

    if(is_set(tlb_set,idx))
    {
        assert(false);
        return addr & 0x1FFF'FFFF;
    }

    else
    {
        // NOTE: this only works because both direct mapped sections 
        // are the same size...
        return addr & 0x1FFF'FFFF;
    }
}


template<typename access_type>
void write_mem_internal(N64& n64, u32 addr, access_type v)
{
    auto& mem = n64.mem;

    // force align addr
    addr &= ~(sizeof(access_type)-1);   

    const u32 idx = addr / PAGE_SIZE;

    if(mem.page_table_write[idx])
    {
        return handle_write_n64<access_type>(mem.page_table_write[idx],addr & (PAGE_SIZE - 1),v);
    }

    // if we are doing a slow access remap the addr manually
    addr = remap_addr(n64,addr);

    write_physical<access_type>(n64,addr,v);    
}

// for now assume accesses are force aligned
// however they are supposed to throw exceptions
// when they are not

template<const b32 debug,typename access_type>
void write_mem(N64 &n64, u32 addr, access_type v)
{
    if constexpr(debug)
    {
#ifdef DEBUG
        if(n64.debug.breakpoint_hit(addr,v,break_type::write))
        {
            write_log(n64.debug,"write breakpoint hit at {:08x}:{:08x}:{:08x}",addr,v,n64.cpu.pc);
            n64.debug.halt();
        }   
#endif
    }

    write_mem_internal<access_type>(n64,addr,v);
}


// for now assume accesses are force aligned
// however they are supposed to throw exceptions
// when they are not

template<typename access_type>
access_type read_mem_internal(N64& n64, u32 addr)
{
    auto& mem = n64.mem;

    // force align addr
    addr &= ~(sizeof(access_type)-1);   

    const u32 idx = addr / PAGE_SIZE;

    if(mem.page_table_read[idx])
    {
        return handle_read_n64<access_type>(mem.page_table_read[idx],addr & (PAGE_SIZE - 1));
    }

    // if we are doing a slow access remap the addr manually
    addr = remap_addr(n64,addr);

    return read_physical<access_type>(n64,addr);    
}

template<const b32 debug,typename access_type>
access_type read_mem(N64 &n64, u32 addr)
{
    const auto v = read_mem_internal<access_type>(n64,addr);

    if constexpr(debug)
    {
#ifdef DEBUG
    if(n64.debug.breakpoint_hit(addr,v,break_type::read))
    {
        write_log(n64.debug,"read breakpoint hit at {:08x}:{:08x}:{:08x}",addr,v,n64.cpu.pc);
        n64.debug.halt();
    }
#endif
    }

    return v;
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

#include "mem/mips_interface.cpp"
#include "mem/rdram.cpp"
#include "mem/sp_regs.cpp"
#include "mem/video_interface.cpp"
#include "mem/peripheral_interface.cpp"
#include "mem/pif.cpp"
#include "mem/serial_interface.cpp"
#include "mem/audio_interface.cpp"