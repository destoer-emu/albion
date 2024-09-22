
#include "playback.h"
#include <algorithm>

#ifdef AUDIO_SDL


#define SDL_MAIN_HANDLED
#ifdef _WIN32
#include <SDL.H>
#else
#include <SDL2/SDL.h>
#endif

// legacy interface
static constexpr SDL_AudioDeviceID dev = 1;

void Playback::init(AudioBuffer& buffer) noexcept
{
    UNUSED(buffer);
    SDL_AudioSpec audio_spec;

	memset(&audio_spec,0,sizeof(audio_spec));

	audio_spec.freq = AUDIO_BUFFER_SAMPLE_RATE;
	audio_spec.format = AUDIO_F32SYS;
	audio_spec.channels = AUDIO_CHANNEL_COUNT;
	audio_spec.samples = 2048;	
	audio_spec.callback = NULL; 
	audio_spec.userdata = NULL;

    if(SDL_OpenAudio(&audio_spec,NULL) < 0) 
    {
        spdlog::error("Failed to open audio {}",SDL_GetError());
    }
	stop();
}

void Playback::start() noexcept
{
	play_audio = true;
    SDL_PauseAudio(0);
}

void Playback::stop() noexcept
{
	play_audio = false;
    SDL_PauseAudio(1);
	SDL_ClearQueuedAudio(dev);
}

Playback::~Playback()
{
    stop();
    SDL_CloseAudio();
}

void push_samples(Playback* playback,AudioBuffer& audio_buffer)
{
    playback->push_samples(audio_buffer);
}

void Playback::push_samples(AudioBuffer& audio_buffer)
{
    if(!play_audio)
    {
        return;
    }

    //printf("pushing: %ld : %ld : %ld\n",audio_buffer.length,audio_buffer.buffer.size(),SDL_GetQueuedAudioSize(dev) / sizeof(f32));

    const u32 buffer_size = audio_buffer.length * sizeof(f32);

    // delay execution and let the queue drain
    while(SDL_GetQueuedAudioSize(dev) > buffer_size)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }			

    
    if(SDL_QueueAudio(dev,audio_buffer.buffer.data(),buffer_size) < 0)
    {
        printf("Failed to queue audio %s\n",SDL_GetError()); exit(1);
    }
}

#else

//external handler
#ifdef AUDIO_ENABLE
static_assert(false,"no audio frontend defined!");
#endif

#endif


// stub audio playback helpers
#ifndef AUDIO_ENABLE
void Playback::start() noexcept
{

}
void Playback::stop() noexcept
{

}

Playback::~Playback() 
{

}

void Playback::push_samples()
{

}
#endif
