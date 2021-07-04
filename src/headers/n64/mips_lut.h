#pragma once
#include <n64/cpu.h>
#include <n64/disass.h>

namespace nintendo64
{

extern const INSTR_FUNC instr_lut[];
extern const DISASS_FUNC disass_lut[];

extern const INSTR_FUNC instr_cop0_lut[];
extern const DISASS_FUNC disass_cop0_lut[];

}