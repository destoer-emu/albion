#pragma once
#include <n64/forward_def.h>
#include <destoer-emu/lib.h>


namespace nintendo64
{


using DISASS_FUNC = std::string (*)(u32 opcode, u64 pc);


std::string disass_opcode(u32 opcode, u64 pc);

std::string disass_lui(u32 opcode, u64 pc);
std::string disass_addiu(u32 opcode, u64 pc);
std::string disass_addi(u32 opcode, u64 pc);
std::string disass_ori(u32 opcode, u64 pc);
std::string disass_andi(u32 opcode, u64 pc);
std::string disass_xori(u32 opcode, u64 pc);
std::string disass_jal(u32 opcode, u64 pc);
std::string disass_slti(u32 opcode, u64 pc);
std::string disass_cache(u32 opcode, u64 pc);

std::string disass_lw(u32 opcode, u64 pc);
std::string disass_sw(u32 opcode, u64 pc);

std::string disass_bne(u32 opcode, u64 pc);
std::string disass_beql(u32 opcode, u64 pc);
std::string disass_bnel(u32 opcode, u64 pc);

std::string disass_sll(u32 opcode, u64 pc);
std::string disass_srl(u32 opcode, u64 pc);
std::string disass_or(u32 opcode, u64 pc);
std::string disass_jr(u32 opcode, u64 pc);
std::string disass_sltu(u32 opcode, u64 pc);

std::string disass_unknown(u32 opcode, u64 pc);
std::string disass_unknown_cop0(u32 opcode, u64 pc);
std::string disass_unknown_r(u32 opcode, u64 pc);

std::string disass_cop0(u32 opcode, u64 pc);
std::string disass_mtc0(u32 opcode, u64 pc); 
std::string disass_r_fmt(u32 opcode, u64 pc);

}