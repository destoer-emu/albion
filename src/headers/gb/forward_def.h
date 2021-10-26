#pragma once
#include <destoer-emu/lib.h>

namespace gameboy
{

struct Memory;
struct Cpu; 
using EXEC_INSTR_FPTR = void (Cpu::*)(void);
struct Ppu;
struct Disass;
struct Apu;
struct GameboyScheduler;
struct GB;
}