#pragma once

namespace nintendo64
{


struct Mem
{
    std::vector<u8> rom;
    std::vector<u8> rd_ram;
    std::vector<u8> sp_dmem;



    // RDRAM interface
    u8 ri_select = 0;
};

void reset_mem(Mem &mem, const std::string &filename);

u8 read_u8(N64 &n64,u32 addr);
u16 read_u16(N64 &n64,u32 addr);
u32 read_u32(N64 &n64,u32 addr);
u64 read_u64(N64 &n64,u32 addr);

}