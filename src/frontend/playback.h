#pragma once
#include <destoer/destoer.h>
#include <albion/audio.h>

class Playback
{
public:
    void init(AudioBuffer& buffer) noexcept;

    bool is_playing() const noexcept { return play_audio; }

    void start() noexcept;
    void stop() noexcept;

    ~Playback();
    void push_samples(AudioBuffer& audio_buffer);

private:
    bool play_audio = false;
};