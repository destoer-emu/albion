#pragma once
#include <n64/mem/mem_constants.h>
#include <n64/mem/sp_regs.h>
#include <n64/mem/rdram_interface.h>
#include <n64/mem/peripheral_interface.h>
#include <n64/mem/video_interface.h>
#include <n64/mem/mips_interface.h>
#include <n64/mem/serial_interface.h>
#include <n64/mem/audio_interface.h>
#include <n64/mem/joybus.h>

namespace nintendo64
{

static constexpr u32 PAGE_SIZE = (1 * 1024 * 1024);
static constexpr u64 MEMORY_SIZE = (u64(4) * 1024 * 1024 * 1024);
static constexpr u64 PAGE_TABLE_SIZE = MEMORY_SIZE / PAGE_SIZE;


struct Mem
{
    std::vector<u8> rom;

    std::vector<u8> rd_ram;

    std::vector<u8> sp_dmem;
    std::vector<u8> sp_imem;

    std::vector<u8> is_viewer;

    u32 rd_ram_regs[10];
    RdramInterface ri;

    SpRegs sp_regs;

    PeripheralInterface pi;
    MipsInterface mi;
    VideoInterface vi;
    SerialInterface si;
    AudioInterface ai;
    Joybus joybus;

    std::vector<u8> pif_ram;

    std::vector<u8*> page_table_read;
    std::vector<u8*> page_table_write;
};

void reset_mem(Mem &mem, const std::string &filename);


template<typename access_type>
access_type read_physical(N64 &n64, u32 addr);

template<typename access_type>
void write_physical(N64 &n64, u32 addr, access_type v);

}