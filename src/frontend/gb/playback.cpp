
#include "playback.h"
#ifdef AUDIO_SDL


#define SDL_MAIN_HANDLED
#ifdef _WIN32
#include <SDL.H>
#else
#include <SDL2/SDL.h>
#endif

void GbPlayback::init(int frequency, int sample_size) noexcept
{
    SDL_AudioSpec audio_spec;

	memset(&audio_spec,0,sizeof(audio_spec));

	audio_spec.freq = frequency;
	audio_spec.format = AUDIO_F32SYS;
	audio_spec.channels = 2;
	audio_spec.samples = sample_size;	
	audio_spec.callback = NULL; // we will use SDL_QueueAudio()  rather than 
	audio_spec.userdata = NULL; // using a callback :)


    SDL_OpenAudio(&audio_spec,NULL);
	SDL_PauseAudio(0);
}

void GbPlayback::mix_samples(float &f1, float &f2, int volume) noexcept
{
    SDL_MixAudioFormat((Uint8*)&f1,(Uint8*)&f2,AUDIO_F32SYS,sizeof(float),volume);
}

void GbPlayback::push_samples(const float *samples, int sample_size)
{
    // legacy interface
    static constexpr SDL_AudioDeviceID dev = 1;
    
    auto buffer_size = (sample_size * sizeof(float));

    // delay execution and let the que drain
    while(SDL_GetQueuedAudioSize(dev) > buffer_size)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }			


    if(SDL_QueueAudio(dev,samples,buffer_size) < 0)
    {
        printf("%s\n",SDL_GetError()); exit(1);
    }
}

void GbPlayback::start() noexcept
{
	play_audio = true;
    SDL_PauseAudio(0);
}

void GbPlayback::stop() noexcept
{
	play_audio = false;
    SDL_PauseAudio(1);
	SDL_ClearQueuedAudio(1);
}

GbPlayback::~GbPlayback()
{
    SDL_CloseAudio();
}


#else
static_assert(false,"no audio frontend defined!");
#endif

