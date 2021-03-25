#pragma once
#ifdef CONTROLLER_SDL

#define SDL_MAIN_HANDLED
#ifdef _WIN32
#include <SDL.H>
#else
#include <SDL2/SDL.h>
#endif

class Controller
{
public:
    void init();
    void connected(int id);
    void disconnected(int id);
    ~Controller();
protected:
    bool controller_connected = false;
    SDL_GameController *controller = NULL;
    int id = -1;
};

#else

// stubbed input does nothing
class Controller
{
    void init() {}
    void connected(int id) {}
    void disconnected(int id) {}
    ~Controller() {}
};

#endif