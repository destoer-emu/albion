namespace nintendo64
{
struct SerialInterface
{
    u32 dram_addr = 0;
    u32 pif_addr = 0;


    // si status , read only

    b32 dma_error = false;
    b32 read_pending = false;

    b32 io_busy = false;
    b32 dma_busy = false;
}; 
}