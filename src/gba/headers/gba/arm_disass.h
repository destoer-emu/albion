#pragma once

#include "forward_def.h"
#include <destoer-emu/lib.h>

class Disass
{
public:
    void init(Mem *mem, Cpu *cpu);

    std::string disass_arm(uint32_t opcode,uint32_t pc);



private:
    Mem *mem;
    Cpu *cpu;
    uint32_t pc; // pc for disassmebling instrs

    using ARM_DISASS_FPTR = std::string (Disass::*)(uint32_t opcode);
    std::vector<ARM_DISASS_FPTR> disass_opcode_table;
    void init_disass_table();

    std::string get_cond_suffix(int opcode);

    std::string disass_branch(uint32_t opcode);
    std::string disass_mov(uint32_t opcode);
    std::string disass_unknown(uint32_t opcode);
    std::string disass_str(uint32_t opcode);
    std::string disass_ldr(uint32_t opcode);
};