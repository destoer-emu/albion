#pragma once
#include "forward_def.h"
#include <destoer-emu/lib.h>


class Disass
{
public:
    void init(Memory *m);
    std::string disass_op(uint16_t addr);
    uint32_t get_op_sz(uint16_t addr);

private:
    Memory *mem;

};