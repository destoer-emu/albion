namespace nintendo64
{
struct PeripheralInterface
{
    u32 cart_addr = 0;
    u32 dram_addr = 0;
    u8 status = 0;
    u32 wr_len = 0;
};

}