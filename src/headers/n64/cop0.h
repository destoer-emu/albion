
struct Status
{
    // cp0 status
    b32 ie;
    b32 exl;
    b32 erl;
    u32 ksu;
    b32 ux;
    b32 sx;
    b32 kx;
    u8 im;
    u32 ds;
    b32 re;
    b32 fr;
    b32 rp;

    // other control bits are unused
    b32 cu1;    
};

struct Cause
{
    u32 code;
    u8 pending;
    u32 coprocessor_error;
    b32 branch_delay;
};

// TODO: factor these into structs
struct Cop0
{
    // cp0 regs

    Status status;

    // cp0 cuase
    Cause cause;

    // count and compare
    u64 count;
    u64 compare; 

    u32 random;

    u32 prid;
    u32 config;

    // cache tags
    u32 tag_hi; 
    u32 tag_lo;
};