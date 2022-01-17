#pragma once


// RDRAM interface
static constexpr u32  RI_SELECT_REG = 0x0470000c;
static constexpr u32  RI_BASE_REG = 0x04700000;
static constexpr u32  RI_CONFIG_REG = 0X04700004;
static constexpr u32  RI_CURRENT_LOAD_REG = 0x04700008;
static constexpr u32  RI_REFRESH_REG = 0x04700010;

// peripheral INTERFACE
static constexpr u32  PI_CART_ADDR_REG = 0x04600004;
static constexpr u32  PI_CART_DRAM_ADDR_REG = 0x04600000;
static constexpr u32  PI_STATUS_REG = 0x04600010;
static constexpr u32  PI_WR_LEN_REG = 0x0460000c;


// mips interface
static constexpr u32 MI_MODE_REG  = 0x04300000;
static constexpr u32 MI_VERSION_REG = 0x04300004;


// video interface
static constexpr u32 VI_CONTROL_REG = 0x04400000;
static constexpr u32 VI_ORIGIN_REG = 0x04400004;
static constexpr u32 VI_WIDTH_REG = 0x04400008;
static constexpr u32 VI_INTR_REG = 0x0440000c;
static constexpr u32 VI_CURRENT_REG = 0x04400010;
static constexpr u32 VI_BURST_REG = 0x04400014;
static constexpr u32 VI_V_SYNC_REG = 0x04400018;
static constexpr u32 VI_H_SYNC_REG = 0x0440001c;
static constexpr u32 VI_LEAP_REG = 0x04400020;
static constexpr u32 VI_H_START_REG = 0x04400024;
static constexpr u32 VI_V_START_REG = 0x04400028;
static constexpr u32 VI_V_BURST_REG = 0x0440002c;
static constexpr u32 VI_X_SCALE_REG = 0x04400030;
static constexpr u32 VI_Y_SCALE_REG = 0x04400034;

// intr bits
static constexpr u32 DP_INTR_BIT = 5;
static constexpr u32 PI_INTR_BIT = 4;
static constexpr u32 VI_INTR_BIT = 3;
