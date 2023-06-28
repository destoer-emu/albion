#pragma once
#include <n64/mem/mem_constants.h>
#include <n64/mem/sp_regs.h>
#include <n64/mem/rdram_interface.h>
#include <n64/mem/peripheral_interface.h>
#include <n64/mem/video_interface.h>
#include <n64/mem/mips_interface.h>
#include <n64/mem/serial_interface.h>
#include <n64/mem/audio_interface.h>

namespace nintendo64
{


struct Mem
{
    std::vector<u8> rom;

    std::vector<u8> rd_ram;

    std::vector<u8> sp_dmem;
    std::vector<u8> sp_imem;


    u32 rd_ram_regs[10];
    RdramInterface ri;

    SpRegs sp_regs;

    PeripheralInterface pi;
    MipsInterface mi;
    VideoInterface vi;
    SerialInterface si;
    AudioInterface ai;

    std::vector<u8> pif_ram;
    b32 joybus_enabled = false;
};

void reset_mem(Mem &mem, const std::string &filename);


template<typename access_type>
access_type read_physical(N64 &n64, u32 addr);

template<typename access_type>
void write_physical(N64 &n64, u32 addr, access_type v);

}