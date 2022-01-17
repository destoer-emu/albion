#pragma once

namespace nintendo64
{


struct Mem
{
    std::vector<u8> rom;

    std::vector<u8> rd_ram;

    std::vector<u8> sp_dmem;
    std::vector<u8> sp_imem;


    // RI
    u8 ri_select = 0;
    u8 ri_config = 0;
    u8 ri_base = 0;
    u32 ri_refresh = 0;

    // PI 
    u32 pi_cart_addr = 0;
    u32 pi_dram_addr = 0;
    u8 pi_status = 0;
    u32 pi_wr_len = 0;

    // MI
    u16 mi_mode = 0;
    u8 mi_intr = 0;

    // VI

    // vi state
    u32 vi_bpp = 0;
    b8 vi_gamma_dither = 0;
    b8 vi_gamma = 0;
    b8 vi_divot = 0;
    b8 vi_serrate = 0;
    u32 vi_aa = 0;

    u32 vi_origin = 0;
    u32 vi_width = 0;
    u32 vi_intr = 0;
    u32 vi_burst = 0;
    u32 vi_vsync = 0;
    u32 vi_hsync = 0;
    u32 vi_leap = 0;
    u32 vi_hstart = 0;
    u32 vi_vstart = 0;
    u32 vi_vburst = 0;
    u32 vi_xscale = 0;
    u32 vi_yscale = 0;
};

void reset_mem(Mem &mem, const std::string &filename);

u8 read_u8(N64 &n64,u32 addr);
u16 read_u16(N64 &n64,u32 addr);
u32 read_u32(N64 &n64,u32 addr);
u64 read_u64(N64 &n64,u32 addr);

void write_u8(N64 &n64,u32 addr,u8 v);
void write_u16(N64 &n64,u32 addr,u16 v);
void write_u32(N64 &n64,u32 addr,u32 v);
void write_u64(N64 &n64,u32 addr,u64 v);

}