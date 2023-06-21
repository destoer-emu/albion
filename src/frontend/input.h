#pragma once
#include <albion/lib.h>
#include <albion/input.h>
#ifdef CONTROLLER_SDL

#define SDL_MAIN_HANDLED
#ifdef _WIN32
#include <SDL.H>
#else
#include <SDL2/SDL.h>
#endif

enum class emu_control
{
    throttle_t,
    unbound_t,
    break_t,
    quit_t,
    none_t,
};

class Input
{
public:
    void init();
    emu_control handle_input(SDL_Window* window);
    void handle_controller_input();
    void add_event_from_key(s32 key, b32 down);

    ~Input();

    Controller controller;

private:
    void connect_controller(int id);
    void disconnect_controller(int id);

    bool controller_connected = false;
    SDL_GameController *game_controller = NULL;
    int id = -1;
};

#else

// stubbed input does nothing
class Input
{
public:
    void init();
    void controller_connected(int id);
    void controller_disconnected(int id);
    ~Input();

    Controller controller;
};

#endif