#include <gba/flash.h>
/*
class Flash
{
    Flash();
    void init(size_t size, const std::string &filename);

    void write_mem(uint32_t addr, uint8_t v);
    uint8_t read_mem(uint32_t addr) const;

    void save_ram();
private:
    enum flash_command_state
    {
        ready,
        command_one,
        command_two,
        chip_identify,
        erase_command,
        erase_chip,
        erase_4k,
        write_byte,
        set_bank
    };



    int bank;

    flash_command_state command_state;

    std::vector<uint8_t> ram;
};
*/

namespace gameboyadvance
{


Flash::Flash()
{
    bank = 0;
    ram.resize(0x10000);
    command_state = flash_command_state::ready;
    write_byte = false;
    erase_command = false;
    chip_identify = false;
    bank_switch = false;
    filename = "";
}

void Flash::init(size_t size, const std::string &rom_name)
{
    ram.resize(size);
    std::fill(ram.begin(),ram.end(),0xff);
    this->filename = rom_name;
    
    command_state = flash_command_state::ready;
    write_byte = false;
    erase_command = false;
    chip_identify = false;
    bank_switch = false;

    // check if there is an existing save for us to load
	if(filename == "")
	{
		return;
	}

	const auto save_name = get_save_file_name(filename);

    auto fp = std::fstream(save_name, std::ios::in | std::ios::binary);
    if(!fp)
    {
        return;
    }

    // toto check for non matching file sizes

    fp.read(reinterpret_cast<char*>(ram.data()), ram.size());
	
    fp.close();	
}


void Flash::write_flash(uint32_t addr, uint8_t v)
{
    addr &= 0xffff;

    if(addr == 0x5555)
    {
        /*
        // not part of any command (not for sayno)
        if(v == 0xf0)
        {
            write_byte = false;
            erase_command = false;
        }
        */
        switch(command_state)
        {
            case flash_command_state::ready:
            {
                if(v == 0xaa)
                {
                    command_state = flash_command_state::command_one;
                }
                break;
            }


            case flash_command_state::command_two:
            {
                switch(v)
                {
                    case 0x90: // enable identify
                    {
                        chip_identify = true;
                        command_state = flash_command_state::ready;
                        break;
                    }

                    case 0xf0: // disable identify
                    {
                        chip_identify = false;
                        command_state = flash_command_state::ready;
                        break;
                    }

                    case 0xb0: // bank switch
                    {
                        bank_switch = true;
                        break;
                    }

                    case 0x80: // wait for erase comamnd
                    {
                        erase_command = true;
                        command_state = flash_command_state::ready;
                        break;
                    }


                    case 0x10: // erase entire chip
                    {
                        for(int i = 0; i < 0x10000; i++)
                        {
                            ram[(bank * 0x10000)+i] = 0xff;
                        }
                        erase_command = false;
                        command_state = flash_command_state::ready;
                        break;
                    }

                    case 0x30:
                    {
                        const auto base = addr & 0xf000;

                        // erase 4k sector
                        for(int i = 0; i < 0x1000; i++)
                        {
                            ram[(bank * 0x10000)+base+i] = 0xff;
                        }
                        erase_command = false;
                        command_state = flash_command_state::ready;
                        break;
                    }

                    case 0xa0: // enable write
                    {
                        write_byte = true;
                        command_state = flash_command_state::ready;
                        break;
                    }

                    // ignore other commands?
                    default:
                    {
                        command_state = flash_command_state::ready;
                        break;
                    }
                }
                break;
            }

            // ignore the write? or reset the state?
            default:
            {
                break;
            }
        }
    }

    else if(addr == 0x2aaa && command_state == flash_command_state::command_one)
    {
        if(v == 0x55)
        {
            command_state = flash_command_state::command_two;
        }
    }

    else
    {
        // can these be triggered in tandem or
        // is this state shared?

        if(bank_switch && addr == 0)
        {
            bank = v & 1;
            // 64kb cant switch bank duh
            if(ram.size() < 0x20000)
            {
                bank = 0;
            }
            bank_switch = false;
        }

        if(write_byte)
        {
            printf("write: %08x:%08x\n",addr,v);
            ram[(bank * 0x10000) + addr] = v;
            write_byte = false;
        }

        if(erase_command && v == 0x30 && command_state == flash_command_state::command_two)
        {
            const auto base = addr & 0xf000;

            // erase 4k sector
            for(int i = 0; i < 0x1000; i++)
            {
                ram[(bank * 0x10000)+base+i] = 0xff;
            }
            erase_command = false;
            command_state = flash_command_state::ready;
        }
    }
}

// investigate what happens when we don non 8 bit accesses on flash
// do they work on real hardware?
uint8_t Flash::read_flash(uint32_t addr)
{
    addr &= 0xffff;

    if(chip_identify && addr < 2)
    {
        return addr == 0? 0x62 : 0x13;
    }

    return ram[(bank * 0x10000) + addr];
}

void Flash::save_ram()
{
	if(filename == "")
	{
		return;
	}


    const auto save_name = get_save_file_name(filename);


    write_file(save_name,ram);
}

}