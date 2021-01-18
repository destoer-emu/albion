#include <gba/flash.h>

namespace gameboyadvance
{


Flash::Flash()
{
    bank = 0;
    ram.resize(0x10000);
    command_state = flash_command_state::ready;
    chip_identify = false;
    filename = "";
}

void Flash::init(size_t size, const std::string &rom_name)
{
    ram.resize(size);
    std::fill(ram.begin(),ram.end(),0xff);
    this->filename = rom_name;
    
    command_state = flash_command_state::ready;
    operation = flash_operation::none;
    chip_identify = false;
    bank = 0;

    // check if there is an existing save for us to load
	if(filename == "")
	{
		return;
	}

    const auto save_name = get_save_file_name(filename);
    try
    {
        read_file(save_name,ram);
    } catch(std::exception &ex) {}
}


void Flash::write_flash(uint32_t addr, uint8_t v)
{
    addr &= 0xffff;

    if(addr == 0x5555 && !(command_state == flash_command_state::ready && v != 0xaa) && command_state != flash_command_state::command_one)
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
                        operation = flash_operation::bank;
                        command_state = flash_command_state::ready;
                        break;
                    }

                    case 0x80: // wait for erase comamnd
                    {
                        operation = flash_operation::erase;
                        command_state = flash_command_state::ready;
                        break;
                    }


                    case 0x10: // erase entire chip
                    {
                        if(operation == flash_operation::erase)
                        {
                            std::fill(ram.begin(),ram.end(),0xff);
                            operation = flash_operation::none;
                            command_state = flash_command_state::ready;
                        }
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
                        operation = flash_operation::none;
                        command_state = flash_command_state::ready;
                        break;
                    }

                    case 0xa0: // enable write
                    {
                        operation = flash_operation::write;
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

            
            default:
            {
                do_flash_operation(addr,v);
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

        else
        {
            do_flash_operation(addr,v);
        }
    }

    else
    {
        do_flash_operation(addr,v);
    }
}

void Flash::do_flash_operation(uint32_t addr, uint8_t v)
{
    // can these be triggered in tandem or
    // is this state shared?

    switch(operation)
    {
        case flash_operation::bank:
        {
            if(addr == 0)
            {  
                bank = v & 1;
                // 64kb cant switch bank duh
                if(ram.size() < 0x20000)
                {
                    bank = 0;
                }
                operation = flash_operation::none;
            }
            break;
        }

        case flash_operation::write:
        {
            //printf("write: %08x:%08x:%08x\n",bank,addr,v);
            ram[(bank * 0x10000) + addr] = v;
            operation = flash_operation::none;
            break;
        }

        case flash_operation::erase: 
        {
            if(v == 0x30 && command_state == flash_command_state::command_two)
            {
                const auto base = addr & 0xf000;

                // erase 4k sector
                for(int i = 0; i < 0x1000; i++)
                {
                    ram[(bank * 0x10000)+base+i] = 0xff;
                }
                operation = flash_operation::none;
                command_state = flash_command_state::ready;
            }
            break;
        }
        
        // do nothing
        case flash_operation::none:
        {
            break;
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