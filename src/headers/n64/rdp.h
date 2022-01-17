#pragma once

namespace nintendo64
{

struct Rdp
{
    u32 screen_x = 0;
    u32 screen_y = 0;
    std::vector<u32> screen;
};



void reset_rdp(Rdp &rdp, u32 x, u32 y);
void change_res(Rdp &rdp, u32 x, u32 y);

}