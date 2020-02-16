#pragma once
#include "forward_def.h"
#include <destoer-emu/lib.h>


namespace gameboy
{

class Disass
{
public:
    void init(Memory *m) noexcept;
    std::string disass_op(uint16_t addr) noexcept;
    uint32_t get_op_sz(uint16_t addr) noexcept;

private:
    Memory *mem = nullptr;

};

}