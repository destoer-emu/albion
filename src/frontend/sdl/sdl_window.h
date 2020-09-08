#pragma once
#ifdef FRONTEND_SDL
#include <gb/gb.h>
#include <gba/gba.h>

#define SDL_MAIN_HANDLED
#ifdef _WIN32
#include <SDL.H>
#else
#include <SDL2/SDL.h>
#endif

class SDLMainWindow
{
public:
    SDLMainWindow(std::string filename);
    ~SDLMainWindow();

private:

    void init_sdl(int x, int y);

    void gameboy_render();
    void gameboy_handle_input();
    void gameboy_main(std::string filename);

    void gba_render();
    void gba_handle_input();
    void gba_main(std::string filename);

    // main emu instance
    gameboy::GB gb;
    gameboyadvance::GBA gba;

    // sdl gfx
	SDL_Window * window = NULL;
	SDL_Renderer * renderer = NULL;
	SDL_Texture * texture = NULL;
    int X;
    int Y;    
};
#endif