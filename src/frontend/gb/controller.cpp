#include "controller.h"
#include <destoer-emu/destoer-emu.h>
using namespace gameboy;

#ifdef CONTROLLER_SDL

void GbControllerInput::init()
{
    // pick first valid controller
	for(int i = 0; i < SDL_NumJoysticks(); i++)
	{
		if(SDL_IsGameController(i))
		{
			controller = SDL_GameControllerOpen(i);
		}

		if(controller == NULL)
		{
			throw std::runtime_error("could not open controller!");
		}

        else
        {
            controller_connected = true;
            break;  
        }
	}  
}

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
        SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
	    SDL_CONTROLLER_BUTTON_RIGHTSHOULDER 
    };

    static constexpr emu_key gb_key[] = 
    {
        emu_key::a,
        emu_key::s,
        emu_key::enter,
        emu_key::space,
        emu_key::minus,
        emu_key::plus
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
    auto x = SDL_GameControllerGetAxis(controller,SDL_CONTROLLER_AXIS_LEFTX);
    auto y = SDL_GameControllerGetAxis(controller,SDL_CONTROLLER_AXIS_LEFTY);

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

    // if something is greater than threshold and not pushed before
    // key press or if it was and now isnt release the key
    // do for all 4 keys


    // right 
    if(x > threshold)
    {
        if(!prev_dpad[RIGHT])
        {
            gb.key_pressed(button::right);
            prev_dpad[RIGHT] = true;
        }
        
        if(prev_dpad[LEFT]) // if left was prev pressed release it
        {
            gb.key_released(button::left);
            prev_dpad[LEFT] = false;
        }
    }

    // left
    else if(x < -threshold)
    {
        if(!prev_dpad[LEFT])
        {
            gb.key_pressed(button::left);
            prev_dpad[LEFT] = true;
        }
        
        if(prev_dpad[RIGHT]) // if right was prev pressed release it
        {
            gb.key_released(button::right);
            prev_dpad[RIGHT] = false;
        }
    }

    // it is in the "deadzone so deset both"
    else
    {
        if(prev_dpad[RIGHT])
        {
            gb.key_released(button::right);
            prev_dpad[RIGHT] = false;
        }
        
        if(prev_dpad[LEFT])
        {
            gb.key_released(button::left);
            prev_dpad[LEFT] = false;
        }			
    }
    

    // up is when its negative?
    if(y < -threshold)
    {
        if(!prev_dpad[UP])
        {
            gb.key_pressed(button::up);
            prev_dpad[UP] = true;
        }
        
        if(prev_dpad[DOWN]) // if down was prev pressed release it
        {
            gb.key_released(button::down);
            prev_dpad[DOWN] = false;
        }
    }

    // down
    else if(y > threshold)
    {
        if(!prev_dpad[DOWN])
        {
            gb.key_pressed(button::down);
            prev_dpad[DOWN] = true;
        }
        
        if(prev_dpad[UP]) // if up was prev pressed release it
        {
            gb.key_released(button::up);
            prev_dpad[UP] = false;
        }
    }

    // in y axis deadzone deset both
    else
    {
        if(prev_dpad[DOWN])
        {
            gb.key_released(button::down);
            prev_dpad[DOWN] = false;
        }
        
        if(prev_dpad[UP])
        {
            gb.key_released(button::up);
            prev_dpad[UP] = false;
        }			
    }
}

GbControllerInput::~GbControllerInput()
{
    if(controller_connected && controller != NULL)
    {
        SDL_GameControllerClose(controller);
        controller = NULL;
    }
}

#endif