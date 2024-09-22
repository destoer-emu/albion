#include <n64/n64.h>

#include "joybus.cpp"

namespace nintendo64
{

static constexpr u32 CONFIG_FLAG = 1 << 0;
static constexpr u32 CHALLENGE_FLAG = 1 << 1;
static constexpr u32 TERMINATE_FLAG = 1 << 3;
static constexpr u32 LOCKOUT_FLAG = 1 << 4;
static constexpr u32 AQUIRE_CHECKSUM_FLAG = 1 << 5;
static constexpr u32 RUN_CHECKSUM_FLAG =  1 << 6;
static constexpr u32 ACK_FLAG = 1 << 7;

void handle_pif_commands(N64& n64)
{
    const u8 commands = handle_read_n64<u8>(n64.mem.pif_ram,PIF_MASK);

    std::string msg("PIF Command: ");

    // joybus config
    if(commands & CONFIG_FLAG)
    {
        auto& joybus = n64.mem.joybus;
        joybus.enabled = true;
    }

    if(commands & CHALLENGE_FLAG)
        msg += "CIC Challenge";

    if(commands & TERMINATE_FLAG)
        msg += "Terminate";

    if(commands & LOCKOUT_FLAG)
        msg += "Lockout Check";

    if(commands & AQUIRE_CHECKSUM_FLAG)
        msg += "Acquire Checksum";

    if(commands & RUN_CHECKSUM_FLAG)
        msg += "Run Checksum";

    spdlog::trace(msg);
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