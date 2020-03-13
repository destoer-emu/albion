#pragma once
#include <gb/gb.h>

#ifdef CONTROLLER_SDL

#define SDL_MAIN_HANDLED
#ifdef _WIN32
#include <SDL.H>
#else
#include <SDL2/SDL.h>
#endif

class GbControllerInput
{
public:
    void init();
    void update(gameboy::GB &gb);
    ~GbControllerInput();
private:
    bool controller_connected = false;
    SDL_GameController *controller = NULL;
};

#else

// stubbed input does nothing
class GbControllerInput
{
    void init() {}
    void update(gameboy::GB &gb) { UNUSED(gb); }
};

#endif