namespace nintendo64
{
struct AudioInterface
{
    u32 dac_rate = 0;
    u32 bit_rate = 0;

    u32 dram_addr = 0;
    u32 length = 0;

    u32 freq = 0;

    // status
    b32 full = false;
    b32 busy = false;
    b32 enabled = false;
};
}