#pragma once

namespace nintendo64
{


struct Mem
{
    std::vector<u8> rom;

    std::vector<u8> rd_ram;

    std::vector<u8> sp_dmem;
    std::vector<u8> sp_imem;


    // sp
    u32 sp_mem_addr = 0;

    u32 sp_pc = 0;
    b32 sp_halt = false;
    b32 sp_broke = false;
    // TODO: is this in the mips intr reg
    b32 sp_intr = false;
    b32 sp_dma_busy = false;
    b32 sp_dma_full = false;
    b32 sp_io_full = false;
    b32 sp_single_step = false;
    b32 sp_clear_intr_on_break = false;
    b32 sp_set_intr_on_break = false;
    b32 intr_on_break = false;
    b32 sp_signal[8] = {0};





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
    u8 mi_mask = 0;

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


}