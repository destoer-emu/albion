#include <gb/rom.h>


void Rom_info::init(std::vector<uint8_t> &rom, std::string romname)
{

    filename = romname;

    // pull the filename
    for(int i = 0x134; i < 0x147; i++)
    {
        filename += rom[i];
    }

    cart_type = rom[0x147];

    has_rtc = false;

    // pull cart type
    switch(cart_type)
    {
        case 0: type = rom_type::rom_only; break; // rom only

        case 1: case 2: case 3:
        { 
            type = rom_type::mbc1; break;
        }

        case 5: case 6:
        { 
            type = rom_type::mbc2; break;
        }
        
        case 10:  type = rom_type::mbc3; has_rtc = true; break; 
        case 0x1b: type = rom_type::mbc5; break;
        default: type = rom_type::mbc3; break; // assume mb3 for now
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
            throw std::runtime_error(fmt::format("invalid bank type: {:x}",bank_type));
        }
    }


    // get the number of ram banks
    uint32_t ram_type = rom[0x149];
    constexpr uint32_t ram_table[5] = {0,1,1,4,16};

    if(ram_type > 4)
    {
        throw std::runtime_error(fmt::format("invalid ram type: {:x}",ram_type));
    }

    no_ram_banks = ram_table[ram_type];
}