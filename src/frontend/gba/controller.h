#pragma once
#include <gba/gba.h>

#ifdef CONTROLLER_SDL

#define SDL_MAIN_HANDLED
#ifdef _WIN32
#include <SDL.H>
#else
#include <SDL2/SDL.h>
#endif

class GbaControllerInput
{
public:
    void init();
    void update(gameboyadvance::GBA &gba);
    ~GbaControllerInput();
private:
    bool controller_connected = false;
    SDL_GameController *controller = NULL;
};

#else

// stubbed input does nothing
class GbaControllerInput
{
    void init() {}
    void update(gameboyadvance::GBA &gba) { UNUSED(gba); }
};

#endif