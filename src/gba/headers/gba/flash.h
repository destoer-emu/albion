#pragma once
#include <gba/forward_def.h>
#include <albion/lib.h>

namespace gameboyadvance
{


struct Flash
{
    Flash();
    void init(size_t size, const std::string &rom_name);


    void write_flash(u32 addr, u8 v);

    u8 read_flash(u32 addr);

    void save_ram();

    void do_flash_operation(u32 addr, u8 v);

    enum class flash_command_state
    {
        ready,
        command_one,
        command_two
    };

    enum class flash_operation
    {
        erase,
        write,
        bank,
        none
    };

    bool chip_identify = false;


    int bank = 0;
    
    std::string filename;

    flash_command_state command_state = flash_command_state::ready;
    flash_operation operation = flash_operation::none;

    std::vector<u8> ram;
};


};