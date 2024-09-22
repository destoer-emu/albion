#pragma once
#include <albion/lib.h>

static constexpr size_t AUDIO_BUFFER_SAMPLE_RATE = 44100;
static constexpr size_t AUDIO_CHANNEL_COUNT = 2;

struct Playback;

struct AudioBuffer 
{
    size_t length;

    /// Audio ring buffer owned
    std::vector<f32> buffer;

    Playback* playback;
};

void push_samples(Playback* playback,AudioBuffer& audio_buffer);


AudioBuffer make_audio_buffer();
void reset_audio_buffer(AudioBuffer& audio_buffer);
size_t audio_buffer_samples(const AudioBuffer& audio_buffer);
void push_sample(AudioBuffer& buffer,f32 left, f32 right);

