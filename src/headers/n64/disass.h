#pragma once
#include <n64/forward_def.h>
#include <destoer-emu/lib.h>


namespace nintendo64
{


using DISASS_FUNC = std::string (*)(u32 opcode, u64 pc);


std::string disass_opcode(u32 opcode, u64 pc);

}