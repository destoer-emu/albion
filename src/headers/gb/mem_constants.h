#pragma once

namespace gameboy
{

constexpr int CART_RAM_BANK_INVALID = -1;


// i/o reg constants only use when performing direct access to the io array(use the actual address otherwhise)
constexpr int IO_NR10 = 0x10;
constexpr int IO_NR11 = 0x11;
constexpr int IO_NR12 = 0x12;
constexpr int IO_NR13 = 0x13;
constexpr int IO_NR14 = 0x14;
constexpr int IO_NR21 = 0x16;
constexpr int IO_NR22 = 0x17;
constexpr int IO_NR23 = 0x18;
constexpr int IO_NR24 = 0x19;
constexpr int IO_NR30 = 0x1a;
constexpr int IO_NR31 = 0x1b;
constexpr int IO_NR32 = 0x1c;
constexpr int IO_NR33 = 0x1d;
constexpr int IO_NR34 = 0x1e;
constexpr int IO_NR41 = 0x20;
constexpr int IO_NR42 = 0x21;
constexpr int IO_NR43 = 0x22;
constexpr int IO_NR44 = 0x23;
constexpr int IO_NR50 = 0x24;
constexpr int IO_NR51 = 0x25;
constexpr int IO_NR52 = 0x26;
constexpr int IO_LY = 0x44;
constexpr int IO_WX = 0x4b;
constexpr int IO_WY = 0x4a;
constexpr int IO_BGP = 0x47;
constexpr int IO_SCY = 0x42;
constexpr int IO_SPEED = 0x4D;
constexpr int IO_VBANK = 0x4f;
constexpr int IO_BGPI = 0x68;
constexpr int IO_BGPD = 0x69;
constexpr int IO_SPPI = 0x6a;
constexpr int IO_SPPD = 0x6b;
constexpr int IO_LCDC = 0x40;
constexpr int IO_STAT = 0x41;
constexpr int IO_LYC = 0x45;
constexpr int IO_DMA = 0x46;
constexpr int IO_IF = 0x0f;
constexpr int IO_IE = 0xff;
constexpr int IO_JOYPAD = 0x00;
constexpr int IO_SB = 0x01;
constexpr int IO_SC = 0x02;
constexpr int IO_BIOS = 0x50;

constexpr int IO_SCX = 0x43;
constexpr int IO_SVBK = 0x70;

// timers
constexpr int IO_TIMA = 0x05;
constexpr int IO_TMA = 0x06;
constexpr int IO_TMC = 0x07;
constexpr int IO_DIV = 0X04;


// cgb dma 
constexpr int IO_HDMA1 = 0x51;
constexpr int IO_HDMA2 = 0x52;

constexpr int IO_HDMA3 = 0x53;
constexpr int IO_HDMA4 = 0x54;

constexpr int IO_HDMA5 = 0x55;

constexpr int IO_PCM12 = 0x76;
constexpr int IO_PCM34 = 0x77;


constexpr int IO_RP = 0x56;

}