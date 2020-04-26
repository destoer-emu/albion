#include <gb/rom.h>
namespace gameboy
{

void fix_checksums(std::vector<uint8_t> &rom)
{
    // fix the checksums

    // header

    uint8_t x = 0;

    for(size_t i = 0x134; i < 0x14d; i++)
    {
        x = x - rom[i] - 1;
    }
    
    rom[0x14d] = x;

    // global
    uint16_t c = 0;
    for(size_t i = 0; i < rom.size(); i++)
    {
        if(i != 0x14e && i != 0x14f)
        {
            c += rom[i];
        }
    }

    rom[0x14e] = (c & 0xff00) >> 8;
    rom[0x14f] = (c & 0xff);    
}

void convert_isx(std::vector<uint8_t> &rom)
{
    // how do we detect extended headers so we can skip past them :P ?


    std::vector<uint8_t> new_rom;

    // no header lets parse it :P
    int len = 0;
    while(len < rom.size())
    {
        switch(rom[len++])
        {
            case 0x1: // binary record
            {
                if(len + 1 >= rom.size())
                {
                    throw std::runtime_error("malformed isx");
                }


                uint8_t bank = rom[len++];

                if(bank >= 0x80)
                {
                    if(len + 1 >= rom.size())
                    {
                        throw std::runtime_error("malformed isx");
                    }
                    // actual bank is in the next byte
                    bank = rom[len++];
                }

                if(len + 4 >= rom.size())
                {
                    throw std::runtime_error("malformed isx");
                }

                uint16_t addr = rom[len++];
                addr |= rom[len++] << 8;
                addr &= 0x3fff;            

                uint16_t data_len = rom[len++];
                data_len |= rom[len++] << 8;

                if(data_len + len >= rom.size())
                {
                    throw std::runtime_error("malformed isx");
                }

                if(addr + data_len > 0x4000)
                {
                    throw std::runtime_error("invalid bank length");
                }


                // if need more space resize to accomodate it 
                if(0x4000 * (bank+1) > new_rom.size())
                {
                    new_rom.resize(0x4000 * (bank+1),0);
                }

                // copy the bank
                memcpy(&new_rom[(bank*0x4000)+addr],&rom[len],data_len);
                len += data_len;
                break;
            }

            case 0x11:
            {
                puts("extend binary");
                exit(1);
            }

            // dont care about anything else we are done
            // may add support for symbol files later
            default: 
            {
                len = rom.size();
            }
        }
    }
    rom = new_rom;

    auto no_banks = rom.size() / 0x4000;

    // fix the header
    // hard code mbc3 & rom size for now

    // find power of two greater than the banks we need
    if(no_banks >= 2)
    {
        size_t banks = 1;

        while(banks < no_banks)
        {
            banks <<= 1;
        }


        no_banks = banks;
        rom[0x148] = static_cast<int>(log2(banks)) - 1;
    }

    else
    {
        no_banks = 2;
        rom[0x148] = 0;
    }



    // add a proper huerstic for the correct mbc type
    // for now we will just assume mbc3 as it works with 
    // the most roms...
    rom[0x147] = 0x10;

    rom.resize(no_banks * 0x4000,0);

    fix_checksums(rom);

    auto fp = std::fstream("converted.gb", std::ios::out | std::ios::binary);
    fp.write(reinterpret_cast<char*>(rom.data()), rom.size());
    fp.close();

}

void Rom_info::init(std::vector<uint8_t> &rom, std::string romname)
{


    // if the name has .isx on the end we need to parse it and convert
    // it to a format our emulator likes
	size_t ext_idx = romname.find_last_of("."); 
	if(ext_idx != std::string::npos)
	{
		std::string ext = romname.substr(ext_idx+1);

        for(auto &x: ext)
        {
            x = tolower(x);
        }

        if(ext == "isx")
        {
            convert_isx(rom);
        }
    }

    filename = romname;


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
        case 8: no_rom_banks = 512; break;
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

    // mbc2 fixed one bank 
    if(type == rom_type::mbc2)
    {
        no_ram_banks = 1;
    }

}

}