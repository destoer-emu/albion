#include <albion/audio.h>

void reset_audio_buffer(AudioBuffer& audio_buffer) 
{
    audio_buffer.length = 0;
    std::fill(audio_buffer.buffer.begin(),audio_buffer.buffer.end(),0.0);
}

AudioBuffer make_audio_buffer() 
{
    AudioBuffer audio_buffer;

    audio_buffer.buffer.resize(2048);
    reset_audio_buffer(audio_buffer);

    return audio_buffer;
}

void push_sample(AudioBuffer& audio_buffer,f32 left, f32 right)
{
    if(audio_buffer.length >= audio_buffer.buffer.size())
    {
        if(audio_buffer.playback)
        {
            push_samples(audio_buffer.playback,audio_buffer);
        }
        audio_buffer.length = 0;
    }
    
    audio_buffer.buffer[audio_buffer.length++] = left;
    audio_buffer.buffer[audio_buffer.length++] = right;
}

size_t audio_buffer_samples(const AudioBuffer& audio_buffer)
{
    return audio_buffer.length / AUDIO_CHANNEL_COUNT;
}
