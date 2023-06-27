
namespace nintendo64
{


struct Status
{
    // cp0 status
    b32 ie = 0;
    b32 exl = 0;
    b32 erl = 0;
    u32 ksu = 0;
    b32 ux = 0;
    b32 sx = 0;
    b32 kx = 0;
    u8 im = 0;
    u32 ds = 0;
    b32 re = 0;
    b32 fr = 0;
    b32 rp = 0;

    // other control bits are unused
    b32 cu1 = 0;    
};

struct Cause
{
    u32 exception_code = 0;
    u8 pending = 0;
    u32 coprocessor_error = 0;
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

    u64 epc = 0;
    u64 error_epc = 0;

    // count and compare
    u64 count = 0;
    u64 compare = 0; 

    u32 random = 0;

    u32 prid = 0;
    u32 config = 0;

    // cache tags (these have fields we just dont care about them)
    u32 tag_hi = 0; 
    u32 tag_lo = 0;
};

}