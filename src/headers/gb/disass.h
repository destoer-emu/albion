#pragma once
#include <gb/forward_def.h>
#include <destoer-emu/lib.h>


namespace gameboy
{

struct Disass
{
    Disass(GB &gb);
    void init() noexcept;
    std::string disass_op(u16 addr) noexcept;
    u32 get_op_sz(u16 addr) noexcept;

    Memory &mem;


    void load_sym_file(const std::string name);
    bool get_symbol(u16 addr, std::string &sym);

    std::vector<std::unordered_map<u16,std::string>> rom_sym_table;
    std::unordered_map<u16,std::string> mem_sym_table;
    bool sym_file_loaded;
};

}