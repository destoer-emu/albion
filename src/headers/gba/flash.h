#pragma once
#include <gba/forward_def.h>
#include <destoer-emu/lib.h>

namespace gameboyadvance
{


class Flash
{
public:
    Flash();
    void init(size_t size, const std::string &rom_name);


    void write_flash(uint32_t addr, uint8_t v);

    uint8_t read_flash(uint32_t addr);

    void save_ram();
private:
    enum flash_command_state
    {
        ready,
        command_one,
        command_two,
    };

    bool chip_identify;
    bool erase_command;
    bool write_byte;
    bool bank_switch;

    int bank;
    
    std::string filename;

    flash_command_state command_state;

    std::vector<uint8_t> ram;
};


};