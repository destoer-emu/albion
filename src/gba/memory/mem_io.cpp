#include <gba/mem_io.h>


MemIo::MemIo()
{
    init();
}

void MemIo::init()
{
    keyinput = 0x3ff;
}