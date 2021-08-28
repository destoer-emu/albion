#pragma once


// RDRAM interface
static constexpr u32  RI_SELECT_REG = 0x0470000c;
static constexpr u32  RI_BASE_REG = 0x04700000;
static constexpr u32  RI_CONFIG_REG = 0X04700004;
static constexpr u32  RI_CURRENT_LOAD_REG = 0x04700008;

// peripheral INTERFACE
static constexpr u32  PI_CART_ADDR_REG = 0x04600004;

// mips interface
static constexpr u32 MI_MODE_REG  = 0x04300000;



// intr bits
static constexpr u32 DP_INTR_BIT = 11;
