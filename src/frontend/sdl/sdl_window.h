#pragma once
#ifdef FRONTEND_SDL
#include <gb/gb.h>
#include <gba/gba.h>
#include <n64/n64.h>
#include <frontend/gb/controller.h>
#include <frontend/gba/controller.h>

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

    void init_sdl(u32 x, u32 y);
    void create_texture(u32 x, u32 y);

    void gameboy_render();
    void gameboy_handle_input(GbControllerInput &controller);
    void gameboy_main(std::string filename);

    void gba_render();
    void gba_handle_input(GbaControllerInput &controller);
    void gba_main(std::string filename);

    void n64_main(std::string filename);
    void n64_render();
    void n64_handle_input();

    // main emu instance
    gameboy::GB gb;
    gameboyadvance::GBA gba;
    nintendo64::N64 n64;

    // sdl gfx
	SDL_Window * window = NULL;
	SDL_Renderer * renderer = NULL;
	SDL_Texture * texture = NULL;
    int X;
    int Y;    
};
#endif