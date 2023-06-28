#include <n64/n64.h>

namespace nintendo64
{

static constexpr u32 CONFIG_FLAG = 1 << 0;
static constexpr u32 CHALLENGE_FLAG = 1 << 1;
static constexpr u32 TERMINATE_FLAG = 1 << 3;
static constexpr u32 LOCKOUT_FLAG = 1 << 4;
static constexpr u32 AQUIRE_CHECKSUM_FLAG = 1 << 5;
static constexpr u32 RUN_CHECKSUM_FLAG =  1 << 6;
static constexpr u32 ACK_FLAG = 1 << 7;

void joybus_comands(N64& n64)
{
    auto& mem = n64.mem;
    auto& si = mem.si;

    // TODO: actually process the commands

    for(u32 i = 0; i < PIF_SIZE; i++)
    {
        handle_write_n64<u8>(mem.pif_ram,i,0xff);
        write_physical<u8>(n64,si.dram_addr+i,0xff);
    }
    handle_write_n64<u8>(mem.pif_ram,PIF_MASK,0x00);
    write_physical<u8>(n64,si.dram_addr+PIF_MASK,0x00);

    n64.mem.joybus_enabled = false;
}

void handle_pif_commands(N64& n64)
{
    const u8 commands = handle_read_n64<u8>(n64.mem.pif_ram,PIF_MASK);

    if(commands & CONFIG_FLAG)
    {
        printf("enable joybus\n");
        n64.mem.joybus_enabled = true;
    }

    if(commands & CHALLENGE_FLAG)
    {
        printf("pif: cic channlge\n");
    }

    if(commands & TERMINATE_FLAG)
    {
        printf("pif: terminate\n");
    }

    if(commands & LOCKOUT_FLAG)
    {
        printf("pif: lockout\n");
    }

    if(commands & AQUIRE_CHECKSUM_FLAG)
    {
        printf("pif: aquire checksum\n");
    }

    if(commands & RUN_CHECKSUM_FLAG)
    {
        printf("pif: run checksum\n");
    }

    handle_write_n64<u8>(n64.mem.pif_ram,PIF_MASK,0);
}


template<typename access_type>
void write_pif(N64& n64, u64 addr, access_type v)
{
    const u64 offset = addr & PIF_MASK;

    handle_write_n64<access_type>(n64.mem.pif_ram,offset,v);

    // command byte written into last byte
    if(offset + sizeof(access_type) == PIF_SIZE)
    {
        handle_pif_commands(n64);
    }
}

template<typename access_type>
access_type read_pif(N64& n64, u64 addr)
{
    const u64 offset = addr & PIF_MASK;

    return handle_read_n64<access_type>(n64.mem.pif_ram,offset);
}

}