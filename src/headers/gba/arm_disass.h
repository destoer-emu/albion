#pragma once

#include "forward_def.h"
#include <albion/lib.h>

namespace gameboyadvance
{

class Disass
{
public:
    void init(Mem *mem, Cpu *cpu);

    std::string disass_arm(u32 opcode,u32 pc);



private:
    Mem *mem = nullptr;
    Cpu *cpu = nullptr;
    u32 pc; // pc for disassmebling instrs

    using ARM_DISASS_FPTR = std::string (Disass::*)(u32 opcode);
    std::vector<ARM_DISASS_FPTR> disass_opcode_table;
    void init_disass_table();

    std::string get_cond_suffix(int opcode);

    std::string disass_branch(u32 opcode);
    std::string disass_mov(u32 opcode);
    std::string disass_unknown(u32 opcode);
    std::string disass_str(u32 opcode);
    std::string disass_ldr(u32 opcode);
};

}