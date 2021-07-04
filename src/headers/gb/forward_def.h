#pragma once
#include <destoer-emu/lib.h>

namespace gameboy
{

class Memory;
class Cpu; 
using EXEC_INSTR_FPTR = void (Cpu::*)(void);
class Ppu;
class Disass;
class Apu;
class GameboyScheduler;
class GB;
}