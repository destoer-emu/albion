#pragma once
#include <n64/forward_def.h>
#include <destoer-emu/lib.h>


namespace nintendo64
{


struct Disass
{

};

using DISASS_FUNC = std::string (*)(N64 &n64,u32 opcode);


std::string disass_opcode(N64 &n64,u32 opcode);

std::string disass_lui(N64 &n64, u32 opcode);
std::string disass_addiu(N64 &n64, u32 opcode);

std::string disass_lw(N64 &n64, u32 opcode);

std::string disass_unknown(N64 &n64, u32 opcode);
std::string disass_unknown_cop0(N64 &n64, u32 opcode);

std::string disass_cop0(N64 &n64, u32 opcode);
std::string disass_mtc0(N64 &n64, u32 opcode); 

}