#pragma once

#include "forward_def.h"
#include <albion/lib.h>
namespace gameboyadvance
{

struct Disass
{
    Disass(GBA &gba);


    std::string disass_arm(uint32_t pc);
    std::string disass_thumb(uint32_t pc);

    Mem &mem;
    Cpu &cpu;
    uint32_t pc; // pc for disassmebling instrs

    const char *shift_names[4] = 
    {
        "lsl",
        "lsr",
        "asr",
        "ror"
    };


    const char *suf_array[16] =
    {
        "eq",
        "ne",
        "cs",
        "cc",
        "mi",
        "pl",
        "vs",
        "vc",
        "hi",
        "ls",
        "ge",
        "lt",
        "gt",
        "le",
        "", // AL
        "" // undefined
    };

    using ARM_DISASS_FPTR = std::string (Disass::*)(uint32_t opcode);
    using THUMB_DISASS_FPTR = std::string (Disass::*)(u16 opcode);
    std::vector<ARM_DISASS_FPTR> disass_arm_table;
    std::vector<THUMB_DISASS_FPTR> disass_thumb_table;
    void init_arm_disass_table();
    void init_thumb_disass_table();

    // arm disassembling
    std::string disass_arm_get_cond_suffix(int opcode);
    std::string disass_arm_branch(uint32_t opcode);
    std::string disass_arm_data_processing(uint32_t opcode);
    std::string disass_arm_unknown(uint32_t opcode);
    std::string disass_arm_single_data_transfer(uint32_t opcode);
    std::string disass_arm_get_shift_string(uint32_t opcode);
    std::string disass_arm_branch_and_exchange(uint32_t opcode);
    std::string disass_arm_psr(uint32_t opcode);
    std::string disass_arm_hds_data_transfer(uint32_t opcode);
    std::string disass_arm_block_data_transfer(uint32_t opcode);
    std::string disass_arm_swap(uint32_t opcode);
    std::string disass_arm_mul(uint32_t opcode);
    std::string disass_arm_mull(uint32_t opcode);
    std::string disass_arm_swi(uint32_t opcode);

    // thumb disassembling
    std::string disass_thumb_ldr_pc(u16 opcode);
    std::string disass_thumb_mov_reg_shift(u16 opcode);
    std::string disass_thumb_unknown(u16 opcode);
    std::string disass_thumb_cond_branch(u16 opcode);
    std::string disass_thumb_mcas_imm(u16 opcode);
    std::string disass_thumb_long_bl(u16 opcode);
    std::string disass_thumb_alu(u16 opcode);
    std::string disass_thumb_add_sub(u16 opcode);
    std::string disass_thumb_multiple_load_store(u16 opcode);
    std::string disass_thumb_hi_reg_ops(u16 opcode);
    std::string disass_thumb_ldst_imm(u16 opcode);
    std::string disass_thumb_push_pop(u16 opcode);
    std::string disass_thumb_load_store_sbh(u16 opcode);
    std::string disass_thumb_load_store_half(u16 opcode);
    std::string disass_thumb_branch(u16 opcode);
    std::string disass_thumb_get_rel_addr(u16 opcode);
    std::string disass_thumb_load_store_reg(u16 opcode);
    std::string disass_thumb_swi(u16 opcode);
    std::string disass_thumb_sp_add(u16 opcode);
    std::string disass_thumb_load_store_sp(u16 opcode);
};

}