#pragma once
#include <destoer-emu/lib.h>

// defines how to push audio samples for the gameboy
class GbPlayback
{
public:
    void init(int frequency,int sample_size) noexcept;

    bool is_playing() const noexcept { return play_audio; }

    void mix_samples(float &f1, float &f2,int volume) noexcept;
    void push_samples(const float *samples, int sample_size);
    void start() noexcept;
    void stop() noexcept;
private:
    bool play_audio = false;

};