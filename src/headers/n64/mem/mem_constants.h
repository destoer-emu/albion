#pragma once


// RDRAM interface
static constexpr u32 RI_SELECT = 0x0470000c;
static constexpr u32 RI_BASE = 0x04700000;
static constexpr u32 RI_CONFIG = 0X04700004;
static constexpr u32 RI_CURRENT_LOAD = 0x04700008;
static constexpr u32 RI_REFRESH = 0x04700010;

// peripheral INTERFACE
static constexpr u32 PI_CART_ADDR = 0x04600004;
static constexpr u32 PI_CART_DRAM_ADDR = 0x04600000;
static constexpr u32 PI_STATUS = 0x04600010;
static constexpr u32 PI_WR_LEN = 0x0460000c;

static constexpr u32 PI_BSD_DOM1_LAT = 0x0460'0014;
static constexpr u32 PI_BSD_DOM1_PWD = 0x0460'0018;
static constexpr u32 PI_BSD_DOM1_PGS = 0x0460'001c;
static constexpr u32 PI_BSD_DOM1_RLS = 0x0460'0020;

static constexpr u32 PI_BSD_DOM2_LAT = 0x0460'0024;
static constexpr u32 PI_BSD_DOM2_PWD = 0x0460'0028;
static constexpr u32 PI_BSD_DOM2_PGS = 0x0460'002c;
static constexpr u32 PI_BSD_DOM2_RLS = 0x0460'0030;

// mips interface
static constexpr u32 MI_MODE  = 0x04300000;
static constexpr u32 MI_VERSION = 0x04300004;
static constexpr u32 MI_INTR_MASK = 0x0430000C;
static constexpr u32 MI_INTERRUPT = 0x0430'0008;


// video interface
static constexpr u32 VI_CONTROL = 0x04400000;
static constexpr u32 VI_ORIGIN = 0x04400004;
static constexpr u32 VI_WIDTH = 0x04400008;
static constexpr u32 VI_INTR = 0x0440000c;
static constexpr u32 VI_CURRENT = 0x04400010;
static constexpr u32 VI_BURST = 0x04400014;
static constexpr u32 VI_V_SYNC = 0x04400018;
static constexpr u32 VI_H_SYNC = 0x0440001c;
static constexpr u32 VI_LEAP = 0x04400020;
static constexpr u32 VI_H_START = 0x04400024;
static constexpr u32 VI_V_START = 0x04400028;
static constexpr u32 VI_V_BURST = 0x0440002c;
static constexpr u32 VI_X_SCALE = 0x04400030;
static constexpr u32 VI_Y_SCALE = 0x04400034;


// serial interface
static constexpr u32 SI_STATUS = 0x0480'0018;

// audio interface
static constexpr u32 AI_STATUS = 0x0450'000C;

// mips interface intr bits
static constexpr u32 SP_INTR_BIT = 0;
static constexpr u32 SI_INTR_BIT = 1;
static constexpr u32 AI_INTR_BIT = 2;
static constexpr u32 VI_INTR_BIT = 3;
static constexpr u32 PI_INTR_BIT = 4;
static constexpr u32 DP_INTR_BIT = 5;




// sp
static constexpr u32 SP_PC = 0x04080000;
static constexpr u32 SP_STATUS = 0x04040010;

static constexpr u32 PIF_SIZE = 0x40;
static constexpr u32 PIF_MASK = PIF_SIZE - 1;