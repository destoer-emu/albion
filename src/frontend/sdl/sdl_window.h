#pragma once
#ifdef FRONTEND_SDL
#include <frontend/input.h>

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
    void main(std::string filename);

protected:
    virtual void init(const std::string& filename) = 0;
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

    b32 throttle_emu;       
};

void start_emu(std::string filename);

#endif