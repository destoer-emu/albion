#pragma once
#include "lib.h"


struct Rom_info
{

    void init(std::vector<uint8_t> &rom)
    {
        // pull the filename
        for(int i = 0x134; i < 0x147; i++)
        {
            filename += rom[i];
        }

        cart_type = rom[0x147];

        // pull cart type
        switch(cart_type)
        {
            case 0: break; // rom only
            case 1 ... 3: mbc1 = true; break;
            case 5 ... 6: mbc2 = true; break;
            case 10:  mbc3 = true; has_rtc = true; break; 
            case 0x1b: mbc5 = true; break;
            default: mbc3 = true; break; // assume mb3 for now
        }

        // get number of rom banks
        int bank_type = rom[0x148];

        switch(bank_type)
        {
            case 0: no_rom_banks = 2; break;
            case 1: no_rom_banks = 4; break;
            case 2: no_rom_banks = 8; break;
            case 3: no_rom_banks = 16; break;
            case 4: no_rom_banks = 32; break;
            case 5: no_rom_banks = 64; break;
            case 6: no_rom_banks = 128; break;
            case 7: no_rom_banks = 256; break;
            case 0x52: no_rom_banks = 72; break;
            case 0x53: no_rom_banks = 80; break;
            case 0x54: no_rom_banks = 96; break;

            default:
            {
                printf("invalid bank type: %x\n",bank_type);
                throw std::runtime_error("invalid bank type");
                exit(1);
            }
        }


        // get the number of ram banks
        uint32_t ram_type = rom[0x149];
        constexpr uint32_t ram_table[5] = {0,1,1,4,16};

        if(ram_type > 4)
        {
            printf("invalid ram type: %x\n",ram_type);
            throw std::runtime_error("invalid ram type");
            exit(1);
        }

        no_ram_banks = ram_table[ram_type];
    }




    int no_ram_banks = 0;
    int no_rom_banks = 0;
    int cart_type = -1;
    std::string filename; 
    bool mbc1 = false;
    bool mbc2 = false;
    bool mbc3 = false;
    bool mbc5 = false;
    bool has_rtc = false;
};