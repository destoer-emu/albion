#pragma once
#include <n64/forward_def.h>
#include <destoer-emu/lib.h>


namespace nintendo64
{


struct Disass
{

};

std::string disass_opcode(N64 &n64,u64 addr);

}