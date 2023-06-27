namespace nintendo64
{
struct SerialInterface
{



    // si status , read only

    b32 dma_error = false;
    b32 read_pending = false;

    b32 io_busy = false;
    b32 dma_busy = false;
}; 
}