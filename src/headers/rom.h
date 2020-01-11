#pragma once
#include "lib.h"


enum class rom_type
{
    mbc1,mbc2,mbc3,mbc5,rom_only
};

struct Rom_info
{
    void init(std::vector<uint8_t> &rom, std::string romname);

    int no_ram_banks = 0;
    int no_rom_banks = 0;
    int cart_type = -1;
    std::string filename; 
    rom_type type;
    bool has_rtc = false;
};