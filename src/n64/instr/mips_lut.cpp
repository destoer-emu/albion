#include <n64/cpu.h>
namespace nintendo64
{
const INSTR_FUNC INSTR_TABLE_DEBUG[] = 
{
	&instr_SPECIAL<true>,
	&instr_REGIMM<true>,
	&instr_j,
	&instr_jal,
	&instr_beq,
	&instr_bne,
	&instr_blez,
	&instr_bgtz,
	&instr_addi,
	&instr_addiu,
	&instr_slti,
	&instr_sltiu,
	&instr_andi,
	&instr_ori,
	&instr_xori,
	&instr_lui,
	&instr_COP0<true>,
	&instr_COP1<true>,
	&instr_COP2<true>,
	&instr_unknown_opcode,
	&instr_beql,
	&instr_bnel,
	&instr_blezl,
	&instr_bgtzl,
	&instr_daddi,
	&instr_daddiu,
	&instr_ldl<true>,
	&instr_ldr<true>,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_lb<true>,
	&instr_lh<true>,
	&instr_lwl<true>,
	&instr_lw<true>,
	&instr_lbu<true>,
	&instr_lhu<true>,
	&instr_lwr<true>,
	&instr_lwu<true>,
	&instr_sb<true>,
	&instr_sh<true>,
	&instr_swl<true>,
	&instr_sw<true>,
	&instr_sdl<true>,
	&instr_sdr<true>,
	&instr_swr<true>,
	&instr_cache,
	&instr_ll<true>,
	&instr_lwc1<true>,
	&instr_lwc2<true>,
	&instr_unknown_opcode,
	&instr_lld<true>,
	&instr_ldc1<true>,
	&instr_ldc2<true>,
	&instr_ld<true>,
	&instr_sc<true>,
	&instr_swc1<true>,
	&instr_swc2<true>,
	&instr_unknown_opcode,
	&instr_scd<true>,
	&instr_sdc1<true>,
	&instr_ldc2<true>,
	&instr_sd<true>,
	&instr_sll,
	&instr_unknown_opcode,
	&instr_srl,
	&instr_sra,
	&instr_sllv,
	&instr_unknown_opcode,
	&instr_srlv,
	&instr_srav,
	&instr_jr,
	&instr_jalr,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_syscall,
	&instr_break,
	&instr_unknown_opcode,
	&instr_sync,
	&instr_mfhi,
	&instr_mthi,
	&instr_mflo,
	&instr_mtlo,
	&instr_dsllv,
	&instr_unknown_opcode,
	&instr_dsrlv,
	&instr_dsrav,
	&instr_mult,
	&instr_multu,
	&instr_div,
	&instr_divu,
	&instr_dmult,
	&instr_dmultu,
	&instr_ddiv,
	&instr_ddivu,
	&instr_add,
	&instr_addu,
	&instr_sub,
	&instr_subu,
	&instr_and,
	&instr_or,
	&instr_xor,
	&instr_nor,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_slt,
	&instr_sltu,
	&instr_dadd,
	&instr_daddu,
	&instr_dsub,
	&instr_dsubu,
	&instr_tge,
	&instr_tgeu,
	&instr_tlt,
	&instr_tltu,
	&instr_teq,
	&instr_unknown_opcode,
	&instr_tne,
	&instr_unknown_opcode,
	&instr_dsll,
	&instr_unknown_opcode,
	&instr_dsrl,
	&instr_dsra,
	&instr_dsll32,
	&instr_unknown_opcode,
	&instr_dsrl32,
	&instr_dsra32,
	&instr_bltz,
	&instr_bgez,
	&instr_bltzl,
	&instr_bgezl,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_tgei,
	&instr_tgeiu,
	&instr_tlti,
	&instr_tltiu,
	&instr_teqi,
	&instr_unknown_opcode,
	&instr_tnei,
	&instr_unknown_opcode,
	&instr_bltzal,
	&instr_bgezal,
	&instr_bltzall,
	&instr_bgezall,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_mfc0,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_mtc0,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_tlbwi,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_tlbp,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_eret,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_cfc1,
	&instr_unknown_opcode,
	&instr_mtc1,
	&instr_unknown_opcode,
	&instr_ctc1,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
};
const INSTR_FUNC INSTR_TABLE_NO_DEBUG[] = 
{
	&instr_SPECIAL<false>,
	&instr_REGIMM<false>,
	&instr_j,
	&instr_jal,
	&instr_beq,
	&instr_bne,
	&instr_blez,
	&instr_bgtz,
	&instr_addi,
	&instr_addiu,
	&instr_slti,
	&instr_sltiu,
	&instr_andi,
	&instr_ori,
	&instr_xori,
	&instr_lui,
	&instr_COP0<false>,
	&instr_COP1<false>,
	&instr_COP2<false>,
	&instr_unknown_opcode,
	&instr_beql,
	&instr_bnel,
	&instr_blezl,
	&instr_bgtzl,
	&instr_daddi,
	&instr_daddiu,
	&instr_ldl<false>,
	&instr_ldr<false>,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_lb<false>,
	&instr_lh<false>,
	&instr_lwl<false>,
	&instr_lw<false>,
	&instr_lbu<false>,
	&instr_lhu<false>,
	&instr_lwr<false>,
	&instr_lwu<false>,
	&instr_sb<false>,
	&instr_sh<false>,
	&instr_swl<false>,
	&instr_sw<false>,
	&instr_sdl<false>,
	&instr_sdr<false>,
	&instr_swr<false>,
	&instr_cache,
	&instr_ll<false>,
	&instr_lwc1<false>,
	&instr_lwc2<false>,
	&instr_unknown_opcode,
	&instr_lld<false>,
	&instr_ldc1<false>,
	&instr_ldc2<false>,
	&instr_ld<false>,
	&instr_sc<false>,
	&instr_swc1<false>,
	&instr_swc2<false>,
	&instr_unknown_opcode,
	&instr_scd<false>,
	&instr_sdc1<false>,
	&instr_ldc2<false>,
	&instr_sd<false>,
	&instr_sll,
	&instr_unknown_opcode,
	&instr_srl,
	&instr_sra,
	&instr_sllv,
	&instr_unknown_opcode,
	&instr_srlv,
	&instr_srav,
	&instr_jr,
	&instr_jalr,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_syscall,
	&instr_break,
	&instr_unknown_opcode,
	&instr_sync,
	&instr_mfhi,
	&instr_mthi,
	&instr_mflo,
	&instr_mtlo,
	&instr_dsllv,
	&instr_unknown_opcode,
	&instr_dsrlv,
	&instr_dsrav,
	&instr_mult,
	&instr_multu,
	&instr_div,
	&instr_divu,
	&instr_dmult,
	&instr_dmultu,
	&instr_ddiv,
	&instr_ddivu,
	&instr_add,
	&instr_addu,
	&instr_sub,
	&instr_subu,
	&instr_and,
	&instr_or,
	&instr_xor,
	&instr_nor,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_slt,
	&instr_sltu,
	&instr_dadd,
	&instr_daddu,
	&instr_dsub,
	&instr_dsubu,
	&instr_tge,
	&instr_tgeu,
	&instr_tlt,
	&instr_tltu,
	&instr_teq,
	&instr_unknown_opcode,
	&instr_tne,
	&instr_unknown_opcode,
	&instr_dsll,
	&instr_unknown_opcode,
	&instr_dsrl,
	&instr_dsra,
	&instr_dsll32,
	&instr_unknown_opcode,
	&instr_dsrl32,
	&instr_dsra32,
	&instr_bltz,
	&instr_bgez,
	&instr_bltzl,
	&instr_bgezl,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_tgei,
	&instr_tgeiu,
	&instr_tlti,
	&instr_tltiu,
	&instr_teqi,
	&instr_unknown_opcode,
	&instr_tnei,
	&instr_unknown_opcode,
	&instr_bltzal,
	&instr_bgezal,
	&instr_bltzall,
	&instr_bgezall,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_mfc0,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_mtc0,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_tlbwi,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_tlbp,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_eret,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_cfc1,
	&instr_unknown_opcode,
	&instr_mtc1,
	&instr_unknown_opcode,
	&instr_ctc1,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
	&instr_unknown_opcode,
};
}
