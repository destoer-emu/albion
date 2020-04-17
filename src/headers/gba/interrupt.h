#pragma once


namespace gameboyadvance
{

enum class interrupt
{
    vblank = 0,
    hblank = 1,
    vcount = 2,
    timer0 = 3,
    timer1 = 4,
    timer2 = 5,
    timer3 = 6,
    serial = 7,
    dma0 = 8,
    dma1 = 9,
    dma2 = 10,
    dma3 = 11,
    keypad = 12,
    gamepak = 13
};

}