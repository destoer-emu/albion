#include <destoer-emu/fps.h>

FpsCounter::FpsCounter()
{
    std::fill(frame_times.begin(),frame_times.end(),1000 / 60);
}

void FpsCounter::reading_start()
{
    start = std::chrono::system_clock::now();
}

void FpsCounter::reading_end()
{
    static auto last = std::chrono::system_clock::now();

    const auto current = std::chrono::system_clock::now();
    auto count = std::chrono::duration_cast<std::chrono::microseconds>(current - start).count();
    if(count == 0) // prevent division crashes
    {
        count = 1;
    }
    frame_times[idx++] = count;
    idx &= SIZE-1;

    // dont report incremental changes
    if(std::chrono::duration_cast<std::chrono::milliseconds>(current - last).count() > 250)
    {
        last = std::chrono::system_clock::now();

    
        const auto frame_time_average = std::accumulate(frame_times.begin(),frame_times.end(),0) / SIZE;

        fps = 1000000 / frame_time_average;
    }
}

uint32_t FpsCounter::get_fps() const
{
    return fps;
}