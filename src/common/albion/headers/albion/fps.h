#pragma once
#include <albion/lib.h>

class FpsCounter
{
public:
    FpsCounter();

    void reading_start();
    void reading_end();

    uint32_t get_fps() const;
private:
    size_t idx = 0;
    uint32_t fps = 0;
    std::chrono::time_point<std::chrono::system_clock> start;
    static constexpr uint32_t SIZE = 64;
    static_assert((SIZE & (SIZE-1)) == 0);
    std::array<uint32_t,SIZE> frame_times;
};