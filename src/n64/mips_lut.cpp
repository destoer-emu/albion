#include <n64/n64.h>

namespace nintendo64 {
const INSTR_FUNC instr_lut[] = {
&instr_r_fmt,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_bne,
&instr_unknown,
&instr_unknown,
&instr_addi,
&instr_addiu,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_ori,
&instr_unknown,
&instr_lui,
&instr_cop0,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_lw,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_sw,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
};


const DISASS_FUNC disass_lut[] = {
&disass_r_fmt,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_bne,
&disass_unknown,
&disass_unknown,
&disass_addi,
&disass_addiu,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_ori,
&disass_unknown,
&disass_lui,
&disass_cop0,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_lw,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_sw,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
};


const INSTR_FUNC instr_cop0_lut[] = {
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_mtc0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
};


const DISASS_FUNC disass_cop0_lut[] = {
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_mtc0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
};


const INSTR_FUNC instr_r_lut[] = {
&instr_sll,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
};


const DISASS_FUNC disass_r_lut[] = {
&disass_sll,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
};


}
