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



template<typename T>
T read_var(size_t &src,std::vector<uint8_t> &buf)
{
    size_t size =  sizeof(T);
    if(src + size >= buf.size())
    {
        throw std::runtime_error("isx: out of range read");
    }
    T var;
    memcpy(&var,&buf[src],size);
    src += size;
    return var;
}

void convert_isx(std::vector<uint8_t> &rom)
{
    puts("converting isx to rom...");

    std::vector<uint8_t> new_rom;
    
    if(rom.size() <= 4)
    {
        throw std::runtime_error("isx: buf too small");
    }

    std::string magic = "";
    for(int i = 0; i < 4; i++)
    {
        magic += static_cast<char>(rom[i]);
    }
    const bool extended = magic == "ISX ";

    // no header lets parse it :P
    // if extended skip header
    size_t len = extended? 0x20: 0;
    while(len < rom.size())
    {
        switch(rom[len++])
        {
            case 0x1: // binary record
            {
                auto bank = read_var<uint8_t>(len,rom);
                if(bank >= 0x80)
                {
                    bank |= read_var<uint8_t>(len,rom) << 8;
                }


                const auto addr = read_var<uint16_t>(len,rom) & 0x3fff;        
                
                auto data_len = read_var<uint16_t>(len,rom);

                if(data_len + len >= rom.size())
                {
                    throw std::runtime_error("malformed isx");
                }

                if(addr + data_len > 0x4000)
                {
                    throw std::runtime_error("invalid bank length");
                }


                // if we need more space resize to accomodate it 
                if(0x4000U * (bank+1) > new_rom.size())
                {
                    new_rom.resize(0x4000 * (bank+1),0);
                }

                // copy the bank
                memcpy(&new_rom[(bank*0x4000)+addr],&rom[len],data_len);
                len += data_len;
                break;
            }

            // symbol
            case 0x4:
            {
                const auto number = read_var<uint16_t>(len,rom);
                for(int i = 0; i < number; i++)
                {
                    const auto sym_len = read_var<uint8_t>(len,rom);
                    len += sym_len + 1; // flag
                    uint16_t bank = read_var<uint8_t>(len,rom);
                    if(bank > 0x80)
                    {
                        bank |= read_var<uint8_t>(len,rom) << 8;
                    }
                    len += 2; // address
                }
                break;
            }

            case 0x11:
            {
                puts("isx extend binary 0x11");
                exit(1);
            }


            case 0x13:
            {
                const auto number = read_var<uint16_t>(len,rom);
                /* // dont know what this is for
                for(int i = 0; i < number; i++)
                {
                    const auto bank = read_var<uint8_t>(len,rom);
                    const auto start = read_var<uint16_t>(len,rom);
                    const auto end = read_var<uint16_t>(len,rom);
                    const auto type = read_var<uint8_t>(len,rom);
                    printf("%x:%x:%x:%x\n",bank,start,end,type);
                }
                */
                len += 9 * number; // block repeated that many times
                break;
            }

            case 0x14: // symbol extended binary
            {
                const auto number = read_var<uint16_t>(len,rom);
                for(int i = 0; i < number; i++)
                {
                    // exta byte on symbol here?
                    const auto sym_len = read_var<uint8_t>(len,rom);
                    len += sym_len + 6; // flag and addr
                }
                break;
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
            try
            {
                convert_isx(rom);
            }

            catch(std::runtime_error &ex)
            {
                throw std::runtime_error(fmt::format("error converting isx!: {}",ex.what()));
            }
        }
    }

    filename = romname;


    cart_type = rom[0x147];

    has_rtc = false;

    // pull cart type
    switch(cart_type)
    {
        case 0x0: case 0x8: case 0x9:
        {
            type = rom_type::rom_only; break; // rom only
        }
        case 0x1: case 0x2: case 0x3:
        { 
            type = rom_type::mbc1; break;
        }

        case 0x5: case 0x6:
        { 
            type = rom_type::mbc2; break;
        }
        
        case 0x10: case 0xF:
        {  
            type = rom_type::mbc3; has_rtc = true; break; 
        }

        case 0x11: case 0x12: case 0x13:
        {  
            type = rom_type::mbc3; break; 
        }

        case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e:
        {
            type = rom_type::mbc5; break;
        }

        default:
        {
            throw std::runtime_error(fmt::format("unknown cart type: {:x}",cart_type));
            break;
        }
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


    if(no_rom_banks != rom.size() / 0x4000)
    {
        puts("[warn] cart header does not match rom size");
        no_rom_banks = rom.size() / 0x4000;
        rom.resize(no_rom_banks*0x4000);
    }


    // get the number of ram banks
    uint32_t ram_type = rom[0x149];
    constexpr uint32_t ram_table[6] = {0,1,1,4,16,8};

    if(ram_type > 5)
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