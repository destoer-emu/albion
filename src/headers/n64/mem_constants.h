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


// intr bits
static constexpr u32 DP_INTR_BIT = 5;
static constexpr u32 PI_INTR_BIT = 4;
