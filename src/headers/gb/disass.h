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

};

}