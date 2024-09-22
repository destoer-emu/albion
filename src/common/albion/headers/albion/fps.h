#pragma once
#include <albion/lib.h>

class FpsCounter
{
public:
    FpsCounter();

    void reading_start();
    void reading_end();

    f32 get_fps() const;
private:
    size_t idx = 0;
    f32 fps = 0;
    std::chrono::time_point<std::chrono::system_clock> start;
    static constexpr uint32_t SIZE = 64;
    static_assert((SIZE & (SIZE-1)) == 0);
    std::array<f32,SIZE> frame_times;
};