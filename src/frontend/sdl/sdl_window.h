#pragma once
#ifdef FRONTEND_SDL
#include <gb/gb.h>
#define SDL_MAIN_HANDLED

#ifdef _WIN32

#include <SDL.H>

#else

#include <SDL2/SDL.h>

#endif

class SDLMainWindow
{
public:
    SDLMainWindow(std::string file_name);
    ~SDLMainWindow();

private:

    void render();
    void init_sdl();
    void handle_input();

    // main emu instance
    GB gb;

    // sdl gfx
	SDL_Window * window;
	SDL_Renderer * renderer;
	SDL_Texture * texture;    
};
#endif