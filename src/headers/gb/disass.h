#pragma once
#include <gb/forward_def.h>
#include <destoer-emu/lib.h>


namespace gameboy
{

class Disass
{
public:
    Disass(GB &gb);
    void init() noexcept;
    std::string disass_op(uint16_t addr) noexcept;
    uint32_t get_op_sz(uint16_t addr) noexcept;

private:
    Memory &mem;


    void load_sym_file(const std::string name);
    bool get_symbol(uint16_t addr, std::string &sym);

    std::vector<std::unordered_map<uint16_t,std::string>> rom_sym_table;
    std::unordered_map<uint16_t,std::string> mem_sym_table;
    bool sym_file_loaded;



};

}