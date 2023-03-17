#include "controller.h"
#include <albion/destoer-emu.h>
using namespace gameboy;

#ifdef CONTROLLER_SDL


void GbControllerInput::update(gameboy::GB &gb)
{
    // no controller we dont care
    if(!controller_connected)
    {
        return;
    }

    // get game controller update
    SDL_GameControllerUpdate();

    static constexpr SDL_GameControllerButton controller_buttons[] = 
    {
        SDL_CONTROLLER_BUTTON_A,
        SDL_CONTROLLER_BUTTON_X,
        SDL_CONTROLLER_BUTTON_START,
        SDL_CONTROLLER_BUTTON_BACK,
        SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
        SDL_CONTROLLER_BUTTON_LEFTSHOULDER
    };

    static constexpr emu_key gb_key[] = 
    {
        emu_key::a,
        emu_key::s,
        emu_key::enter,
        emu_key::space,
        emu_key::k,
        emu_key::l
    };
    static_assert(sizeof(controller_buttons) == sizeof(gb_key));


    static constexpr int CONTROLLER_BUTTONS_SIZE = sizeof(controller_buttons) / sizeof(controller_buttons[0]);

    // cache the old state so we know when a new key press has occured
    static bool buttons_prev[CONTROLLER_BUTTONS_SIZE] = {false};

    // check the controller button state
    for(int i = 0; i < CONTROLLER_BUTTONS_SIZE; i++)
    {
        // now we handle controller inputs
        auto b = SDL_GameControllerGetButton(controller,controller_buttons[i]);
        if(b && !buttons_prev[i])
        {
            gb.key_input(static_cast<int>(gb_key[i]),true);
        }

        else if(!b && buttons_prev[i])
        {
            gb.key_input(static_cast<int>(gb_key[i]),false);
        }
        buttons_prev[i] = b;
    }


    // handle the joystick
    const auto x = SDL_GameControllerGetAxis(controller,SDL_CONTROLLER_AXIS_LEFTX);
    const auto y = SDL_GameControllerGetAxis(controller,SDL_CONTROLLER_AXIS_LEFTY);

    // input of more than half in either direction is enough to make
    // to cause an input
    constexpr int16_t threshold = std::numeric_limits<int16_t>::max() / 2;


    static constexpr int LEFT = 0;
    static constexpr int RIGHT = 1;
    static constexpr int UP = 2;
    static constexpr int DOWN = 3;
    static bool prev_dpad[4] = 
    {
        false, // left
        false, // right
        false, // up
        false // down
    };


    // in y axis deadzone deset both
    if(y == threshold)
    {
        if(prev_dpad[DOWN])
        {
            gb.key_input(static_cast<int>(emu_key::down),false);
        }

        if(prev_dpad[UP])
        {
            gb.key_input(static_cast<int>(emu_key::up),false);
        }
        prev_dpad[DOWN] = false;
        prev_dpad[UP] = false;
    }

    // in x axis deadzone deset both
    if(x == threshold)
    {
        if(prev_dpad[LEFT])
        {
            gb.key_input(static_cast<int>(emu_key::left),false);
        }

        if(prev_dpad[RIGHT])
        {
            gb.key_input(static_cast<int>(emu_key::right),false);
        }
        prev_dpad[LEFT] = false;
        prev_dpad[RIGHT] = false;
    }

    const bool r = x > threshold;
    const bool l = x < -threshold;
    const bool u = y < -threshold;
    const bool d = y > threshold;



    // right
    if(r != prev_dpad[RIGHT])
    {
        gb.key_input(static_cast<int>(emu_key::right),r);
        prev_dpad[RIGHT] = r;
    }

    // left
    if(l != prev_dpad[LEFT])
    {
        gb.key_input(static_cast<int>(emu_key::left),l);
        prev_dpad[LEFT] = l;
    }

    // up
    if(u != prev_dpad[UP])
    {
        gb.key_input(static_cast<int>(emu_key::up),u);
        prev_dpad[UP] = u;    
    }

    // down
    if(d != prev_dpad[DOWN])
    {
        gb.key_input(static_cast<int>(emu_key::down),d);
        prev_dpad[DOWN] = d;
    }
    
}

#endif