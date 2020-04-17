#pragma once
#include <destoer-emu/lib.h>

// defines how to push audio samples for the gameboy
class GbPlayback
{
public:
    void init(int playback_frequency,int sample_size) noexcept;

    bool is_playing() const noexcept { return play_audio; }

    void mix_samples(float &f1, float &f2,int volume) noexcept;
    void push_sample(const float &l, const float &r) noexcept;
    void start() noexcept;
    void stop() noexcept;

    ~GbPlayback();
private:
    void push_samples();

    bool play_audio = false;


    size_t sample_idx = 0;

	// sound playback
    std::vector<float> audio_buf;
};