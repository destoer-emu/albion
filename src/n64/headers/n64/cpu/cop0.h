
namespace nintendo64
{


struct Status
{
    // cp0 status
    b32 ie = 0;
    b32 exl = 0;
    b32 erl = 0;
    u32 ksu = 0;
    b32 ux = 1;
    b32 sx = 1;
    b32 kx = 1;
    u8 im = 0;
    u32 ds = 0b10000;
    b32 re = 0;
    b32 fr = 1;
    b32 rp = 0;

    b32 cu0 = false;
    b32 cu1 = true;
    b32 cu2 = false;
    b32 cu3 = false;    
};

struct Cause
{
    u32 exception_code = 0;
    u8 pending = 0;
    u32 coprocessor_error = 0b11;
    b32 branch_delay = 0;
};

struct EntryHi
{
    // entry hi
    u32 vpn2 = 0;
    u32 asid = 0;
};

struct EntryLo
{
    u32 pfn = 0;
    u32 c = 0;
    b32 d = 0;
    b32 v = 0;
    b32 g = 0;
};

struct Index
{
    b32 p;
    u32 idx;
};

struct Context
{
    u32 bad_vpn2 = 0;
    u32 pte_base = 0;
};

struct Config {
    u8 freq = 0b111;
    u8 transfer_mode = 0;
    u8 endianness = 1;
    u8 cu = 0;
    u8 k0 = 0b11;
};

struct WatchLo {
    u32 paddr0;
    u8 read;
    u8 write;
};

struct WatchHi {
    u8 paddr1;
};

struct XConfig {
    u32 pte;
    u8 r;
    u32 bad_vpn;
};

// TODO: factor these into structs
struct Cop0
{
    // cp0 regs

    Status status;

    // cp0 cuase
    Cause cause;

    // NOTE: tlb entrys assumes 32 bit adressing
    EntryHi entry_hi;
    EntryLo entry_lo_one;
    EntryLo entry_lo_zero;
    Index index;
    u32 page_mask = 0;
    u64 bad_vaddr = 0;
    Context context;

    b32 ll_bit = false;

    u64 epc = 0;
    u64 error_epc = 0;

    // count and compare
    u32 count = 0;
    u32 compare = 0;

    u8 wired = 0;

    // random register set to 31 on init
    u32 random = 0b11111;

    u32 prid = 0xB22;
    Config config;

    // cache tags (these have fields we just dont care about them)
    WatchHi watchHi;
    WatchLo watchLo;

    u32 tagLo;

    u8 parity = 0;

    XConfig xconfig;

    u32 load_linked = ~0u;

    void updateRandom();
};

static constexpr u32 COUNT_BIT = 7;
static constexpr u32 MI_BIT = 2;

}