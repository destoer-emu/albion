
#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <map>
#include <vector>
#include <set>
#include <format>
#include <cstring>

enum MIPS_VER
{
    MIPS1,
    MIPS2,
    MIPS3,
    MIPS4,
};

// integer typedefs
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using b32 = bool;
using b8 = u8;

using f32 = float;
using f64 = double;

static constexpr u32 INSTR_TYPE_MASK = 0b111'111;
static constexpr u32 FUNCT_MASK = 0b111'111;
static constexpr u32 REGIMM_MASK = 0b111'11;
static constexpr u32 COP0_RS_MASK = 0b111'11;
static constexpr u32 COP0_FUNCT_MASK = 0b111'111;

// base offsets
static constexpr u32 BASE_OFFSET = 0;
static constexpr u32 SPECIAL_OFFSET = BASE_OFFSET + INSTR_TYPE_MASK + 1;
static constexpr u32 REGIMM_OFFSET = SPECIAL_OFFSET + FUNCT_MASK + 1;


// cop0 offsets
static constexpr u32 COP0_RS_OFFSET = REGIMM_OFFSET + REGIMM_MASK + 1;
static constexpr u32 COP0_FUNCT_OFFSET = COP0_RS_OFFSET + COP0_RS_MASK + 1;
static constexpr u32 COP0_END = COP0_FUNCT_OFFSET + COP0_FUNCT_MASK + 1;



// cop1 offsets
static constexpr u32 COP1_RS_SHIFT = 21;
static constexpr u32 COP1_RS_MASK = 0b111'11;

static constexpr u32 COP1_BC_MASK = 0b111'11;
static constexpr u32 COP1_BC_SHIFT = 16;

static constexpr u32 COP1_FMT_SHIFT = 0;
static constexpr u32 COP1_FMT_MASK = 0b111'111;

static constexpr u32 COP1_RS_OFFSET = COP0_END;
static constexpr u32 COP1_BC_OFFSET = COP1_RS_OFFSET + COP1_RS_MASK + 1;

static constexpr u32 COP1_FMT_S_OFFSET = COP1_BC_OFFSET + COP1_BC_MASK + 1;
static constexpr u32 COP1_FMT_D_OFFSET = COP1_FMT_S_OFFSET + COP1_FMT_MASK + 1;
static constexpr u32 COP1_FMT_W_OFFSET = COP1_FMT_D_OFFSET + COP1_FMT_MASK + 1;
static constexpr u32 COP1_FMT_L_OFFSET = COP1_FMT_W_OFFSET + COP1_FMT_MASK + 1;

static constexpr u32 LAST_OFFSET = COP1_FMT_L_OFFSET;
static constexpr u32 LAST_MASK = COP1_FMT_MASK;

// NOTE: generally every 4 or 8 instrs have the same format.

// NOTE: this is kept as one contiguous table rather than seperate ones to keep it easy to dump and to avoid fracturing memory

// NOTE: extra entry for chain failure
static constexpr u32 INSTR_TABLE_SIZE = LAST_OFFSET + LAST_MASK + 2;


u32 get_opcode_type(u32 opcode)
{
    return (opcode >> 26) & INSTR_TYPE_MASK;
}

enum class instr_type
{
    reg_rd_rs_rt,
    reg_rd_rt_rs,
    reg_rs_rt,
    reg_rd_rs,
    reg_rd_rt,
    reg_rt_rd,
    reg_rd,
    reg_rs,
    shift,

    trap,

    imm_signed,
    imm_unsigned,
    imm_rs,

    store,
    store_float,
    store_cop2,

    load,
    load_float,
    load_cop2,

    branch_rs_rt,
    branch_rs,
    branch_reg,
    jump,

    bit_op,

    float_rt_fs,
    float_fd_fs,
    float_fd_fs_ft,
    float_fs_ft,
    branch_cop_cond,

    // note: not a real fmt
    mips_class,

    unk,
};

b32 is_mem_access(instr_type type)
{
    switch(type)
    {
        case instr_type::store: return true;
        case instr_type::store_float: return true;
        case instr_type::store_cop2: return true;

        case instr_type::load: return true;
        case instr_type::load_float: return true;
        case instr_type::load_cop2: return true;

        default: return false;
    }
}

struct Opcode
{
    u32 op = 0;
    u32 type = 0;
    u32 rd = 0;
    u32 rs = 0;
    u32 rt = 0;
    u16 imm = 0;
};

struct Program;
using READ_FUNC = b32 (*)(Program& program, u64 addr, void* out,u32 size);

static constexpr u32 SIZE_UNK = 0xffff'ffff;

static constexpr u64 PC_UNK = 0xffff'ffff'ffff'ffff;

struct Func
{
    std::string name;
    u64 addr = 0;
    u32 size = SIZE_UNK;

    std::vector<u64> references;

    // sorted blocks
    std::set<u64> block_list;

    // which local branches havent been parsed yet?
    std::vector<u64> branch_target;

    b32 external = false;
};

struct Block
{
    u64 func = 0;

    u64 addr = 0;
    u32 size = 0;

    // what other blocks does this exit to?
    std::vector<u64> exit;

    std::vector<u64> references;

    // default to LOC_%08x but some may have proper names
    std::string name;

    u64 hash = 0;
};

// TODO: need a way to get a function by block from an arbitary address
// but we aernt going to worry about this just yet
struct Program
{
    // program entry point
    u64 entry_point = 0;

    // TODO: replace this with a more generic abstract interpretation mechanism for now
    // just hard code this
    u64 gp = 0;

    // what endianess is the program?
    b32 is_be = false;

    // function to perfrom address translation
    READ_FUNC read_func;

    void* data;

    // what targets have not been checked yet?
    std::vector<u64> func_target;

    // list of functions
    std::map<u64,Func> func_lookup;

    std::map<std::string,u64> func_name_lookup;

    // list of blocks
    std::map<u64, Block> block_lookup;
};


struct Instr;
using CHAIN_FUNC = const Instr* (*)(const Opcode& opcode, u32 version);

using DISASS_FUNC = std::string (*)(Program& program, u64 addr, const Opcode& opcode);

// return true if block is finished!
using MARK_FUNC = b32 (*)(Program& program,Func& func,  Block& block, u64 addr, const Opcode& opcode);



struct Instr
{
    const char *name;
    instr_type fmt;
    MARK_FUNC mark_func;
    DISASS_FUNC disass_func;

    // what version was this added in
    u32 version;

    // fptr that will give back the correct instr field
    CHAIN_FUNC chain = nullptr;
};

#define UNUSED(a) (void)a


b32 unknown_mark_err(Program& program,Func& func,  Block& block, u64 pc, const Opcode& opcode)
{
    UNUSED(program); UNUSED(pc); UNUSED(opcode); UNUSED(func); UNUSED(block);
    return true;
}

std::string fmt_opcode(const Opcode& opcode)
{
    return "";
}

// TODO: note this is a debugging handler when we hit an actual unknown up
// we will just mark it!
std::string unknown_disass(Program& program, u64 addr, const Opcode& opcode)
{
    printf("[UNKNOWN BASE OPCODE] 0x{:08x}: {}\n",addr, fmt_opcode(opcode));

    UNUSED(program); UNUSED(addr); UNUSED(opcode);
    return "";
}

std::string unknown_disass_cop0(Program& program, u64 addr, const Opcode& opcode)
{
    UNUSED(program); UNUSED(addr); UNUSED(opcode);

    return "cop0_unknown";
}


std::string unknown_disass_cop1(Program& program, u64 addr, const Opcode& opcode)
{
    UNUSED(program); UNUSED(addr); UNUSED(opcode);

    return "cop1_unknown";
}

std::string unknown_disass_cop1x(Program& program, u64 addr, const Opcode& opcode)
{
    UNUSED(program); UNUSED(addr); UNUSED(opcode);

    return "cop1x_unknown";
}

std::string unknown_disass_cop2(Program& program, u64 addr, const Opcode& opcode)
{
    UNUSED(program); UNUSED(addr); UNUSED(opcode);

    return "cop2_unknown";
}


static constexpr u32 REGIMM_SHIFT = 16;
static constexpr u32 SPECIAL_SHIFT = 0;
static constexpr u32 BASE_SHIFT = 26;

std::string unknown_disass_special(Program& program, u64 addr, const Opcode& opcode)
{
    printf("[UNKNOWN SPECIAL OPCODE] 0x{:08x}: {}, funct: 0b{:06b}\n",addr, fmt_opcode(opcode), (opcode.op >> 0) & FUNCT_MASK);

    UNUSED(program); UNUSED(addr); UNUSED(opcode);
    return "";
}

b32 mark_bltzl(Program& program,Func& func,  Block& block, u64 pc, const Opcode& opcode) {return 0;}
b32 mark_jump(Program& program,Func& func,  Block& block, u64 pc, const Opcode& opcode) {return 0;}
b32 mark_jal(Program& program,Func& func,  Block& block, u64 pc, const Opcode& opcode) {return 0;}
b32 mark_beq(Program& program,Func& func,  Block& block, u64 pc, const Opcode& opcode) {return 0;}
b32 mark_bne(Program& program,Func& func,  Block& block, u64 pc, const Opcode& opcode) {return 0;}
b32 mark_blez(Program& program,Func& func,  Block& block, u64 pc, const Opcode& opcode) {return 0;}
b32 mark_bgtz(Program& program,Func& func,  Block& block, u64 pc, const Opcode& opcode) {return 0;}
b32 mark_beql(Program& program,Func& func,  Block& block, u64 pc, const Opcode& opcode) {return 0;}
b32 mark_bnel(Program& program,Func& func,  Block& block, u64 pc, const Opcode& opcode) {return 0;}
b32 mark_blezl(Program& program,Func& func,  Block& block, u64 pc, const Opcode& opcode) {return 0;}
b32 mark_bgezl(Program& program,Func& func,  Block& block, u64 pc, const Opcode& opcode) {return 0;}
b32 mark_jr(Program& program,Func& func,  Block& block, u64 pc, const Opcode& opcode) {return 0;}
b32 mark_bgtzl(Program& program,Func& func,  Block& block, u64 pc, const Opcode& opcode) {return 0;}
b32 mark_bgezal(Program& program,Func& func,  Block& block, u64 pc, const Opcode& opcode) {return 0;}
b32 mark_jalr(Program& program,Func& func,  Block& block, u64 pc, const Opcode& opcode) {return 0;}
b32 mark_bltz(Program& program,Func& func,  Block& block, u64 pc, const Opcode& opcode) {return 0;}
b32 mark_bgez(Program& program,Func& func,  Block& block, u64 pc, const Opcode& opcode) {return 0;}
b32 mark_bltzal(Program& program,Func& func,  Block& block, u64 pc, const Opcode& opcode) {return 0;}
b32 mark_bgezall(Program& program,Func& func,  Block& block, u64 pc, const Opcode& opcode) {return 0;}
b32 mark_bltzall(Program& program,Func& func,  Block& block, u64 pc, const Opcode& opcode) {return 0;}


std::string disass_beq(Program& program, u64 addr, const Opcode& opcode) { return "";}
std::string disass_li(Program& program, u64 addr, const Opcode& opcode) { return "";}
std::string disass_lui(Program& program, u64 addr, const Opcode& opcode) { return "";}
std::string disass_cop0(Program& program, u64 addr, const Opcode& opcode) { return "";}
std::string disass_cop1(Program& program, u64 addr, const Opcode& opcode) { return "";}
std::string disass_cache(Program& program, u64 addr, const Opcode& opcode) { return "";}
std::string disass_pref(Program& program, u64 addr, const Opcode& opcode) { return "";}
std::string disass_sll(Program& program, u64 addr, const Opcode& opcode) { return "";}
std::string disass_jr(Program& program, u64 addr, const Opcode& opcode) { return "";}
std::string disass_jalr(Program& program, u64 addr, const Opcode& opcode) { return "";}
std::string disass_break(Program& program, u64 addr, const Opcode& opcode) { return "";}
std::string disass_sync(Program& program, u64 addr, const Opcode& opcode) { return "";}
std::string disass_addu(Program& program, u64 addr, const Opcode& opcode) { return "";}
std::string disass_or(Program& program, u64 addr, const Opcode& opcode) { return "";}
std::string disass_nor(Program& program, u64 addr, const Opcode& opcode) { return "";}
std::string disass_bgezal(Program& program, u64 addr, const Opcode& opcode) { return "";}
std::string disass_mfc0(Program& program, u64 addr, const Opcode& opcode) { return "";}
std::string disass_mtc0(Program& program, u64 addr, const Opcode& opcode) { return "";}

const Instr* decode_instr(const Opcode& opcode, u32 version);
const Instr* decode_regimm(const Opcode& opcode,u32 version);
const Instr* decode_special(const Opcode& opcode,u32 version);
const Instr* decode_cop0(const Opcode& opcode, u32 version);
const Instr* decode_cop1(const Opcode& opcode, u32 version);

#include <mips_table.inl>

static constexpr Instr const* BASE_TABLE = &INSTR_TABLE[BASE_OFFSET];
static constexpr Instr const* SPECIAL_TABLE = &INSTR_TABLE[SPECIAL_OFFSET];
static constexpr Instr const* REGIMM_TABLE = &INSTR_TABLE[REGIMM_OFFSET];

const Instr* decode_special(const Opcode& opcode,u32 version)
{
    const u32 idx = (opcode.op >> SPECIAL_SHIFT) & FUNCT_MASK;

    const Instr* instr = &SPECIAL_TABLE[idx];

    if(instr->chain)
    {
        return instr->chain(opcode,version);
    }

    return instr;
}

const Instr* decode_regimm(const Opcode& opcode,u32 version)
{
    const u32 idx = (opcode.op >> REGIMM_SHIFT) & REGIMM_MASK;

    const Instr* instr = &REGIMM_TABLE[idx];

    if(instr->chain)
    {
        return instr->chain(opcode,version);
    }

    return instr;
}


const Instr* decode_cop1(const Opcode& opcode, u32 version)
{
    return &INSTR_TABLE[0];
}

const Instr* decode_cop0(const Opcode& opcode, u32 version)
{
    return &INSTR_TABLE[0];
}

#include <table_gen.cpp>

int main(int argc, char** argv) {
    if (argc < 1) {
        std::cout << "usage: lutgen <filename>" << std::endl;
        return 1;
    }

    beyond_all_repair::gen_mips_table(argv[1]);

    return 0;
}