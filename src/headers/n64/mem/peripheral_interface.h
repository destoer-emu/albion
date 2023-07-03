namespace nintendo64
{
struct PeripheralInterface
{
    u32 cart_addr = 0;
    u32 dram_addr = 0;
    u32 wr_len = 0;
    b32 busy = false;

    // memory access contorls?
    u32 bsd_dom1_lat = 0;
    u32 bsd_dom1_pwd = 0;
    u32 bsd_dom1_pgs = 0;
    u32 bsd_dom1_rls = 0;

    u32 bsd_dom2_lat = 0;
    u32 bsd_dom2_pwd = 0;
    u32 bsd_dom2_pgs = 0;
    u32 bsd_dom2_rls = 0;
};

}