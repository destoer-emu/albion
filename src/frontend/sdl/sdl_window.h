#pragma once
#ifdef FRONTEND_SDL
#include <frontend/input.h>
#include <frontend/playback.h>

#define SDL_MAIN_HANDLED
#ifdef _WIN32
#include <SDL.H>
#else
#include <SDL2/SDL.h>
#endif


class SDLMainWindow
{
public:
    ~SDLMainWindow();
    void main(std::string filename, b32 start_debug);

protected:
    // This should setup the playback with an appropiate buffer
    virtual void init(const std::string& filename,Playback& playback) = 0;
    virtual void pass_input_to_core() = 0;
    virtual void run_frame() = 0;
    virtual void handle_debug() = 0;
    virtual void core_quit() = 0;
    virtual void core_throttle() = 0;
    virtual void core_unbound() = 0;
    virtual void debug_halt() = 0;


    void init_sdl(u32 x, u32 y);
    void create_texture(u32 x, u32 y); 
    void render(const u32* data);

    // sdl gfx
	SDL_Window * window = NULL;
	SDL_Renderer * renderer = NULL;
	SDL_Texture * texture = NULL;
    s32 X;
    s32 Y;

    Input input;
    Playback playback;

    b32 throttle_emu;       
};


// only supported on SDL for now
struct Config
{
    b32 start_debug = false;
};

inline Config get_config(int argc, char* argv[])
{
    Config cfg;

    if(argc == 3)
    {
        const char* str = argv[2];
        while(*str)
        {
            const char c = *str;

            switch(c)
            {
                case 'd': cfg.start_debug = true; break;
                case '-': break;
                default: printf("warning unknown flag: %c\n",c);
            }

            str++;
        }
    }

    return cfg;    
}


void start_emu(std::string filename, Config& cfg);

#endif