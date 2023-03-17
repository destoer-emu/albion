#pragma once
#include <n64/forward_def.h>
#include <albion/lib.h>


namespace nintendo64
{


using DISASS_FUNC = std::string (*)(const Opcode &opcode, u64 pc);


std::string disass_opcode(const Opcode &opcode, u64 pc);

}