#pragma once

namespace nintendo64
{

struct Rdp
{
    u32 screen_x = 0;
    u32 screen_y = 0;
    std::vector<u32> screen;

    u32 ly = 0;
    u32 line_cycles = 0;
    u32 scan_lines = 525;

    bool frame_done;
};



void reset_rdp(N64 &n64);
void change_res(N64 &n64);

static constexpr u32 VIDEO_CLOCK = 46 * 1024 * 1024;

}