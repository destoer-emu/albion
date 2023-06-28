

namespace nintendo64
{

struct SpRegs
{
    // sp
    u32 mem_addr = 0;

    u32 pc = 0;
    b32 halt = false;
    b32 broke = false;


    b32 dma_busy = false;
    b32 dma_full = false;
    b32 io_full = false;
    b32 single_step = false;
    b32 clear_intr_on_break = false;
    b32 set_intr_on_break = false;
    b32 intr_on_break = false;
    b32 signal[8] = {0};

};

}