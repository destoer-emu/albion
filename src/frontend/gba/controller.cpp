#include "controller.h"
#include <destoer-emu/destoer-emu.h>
using namespace gameboyadvance;

#ifdef CONTROLLER_SDL

// todo rewrite input system for when we do n64
void GbaControllerInput::init()
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

void GbaControllerInput::update(gameboyadvance::GBA &gba)
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

    static constexpr emu_key gba_key[] = 
    {
        emu_key::a,
        emu_key::s,
        emu_key::enter,
        emu_key::space,
        emu_key::k,
        emu_key::l
    };
    static_assert(sizeof(controller_buttons) == sizeof(gba_key));


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
            gba.key_input(static_cast<int>(gba_key[i]),true);
        }

        else if(!b && buttons_prev[i])
        {
            gba.key_input(static_cast<int>(gba_key[i]),false);
        }
        buttons_prev[i] = b;
    }


    // handle the joystick
    const auto x = SDL_GameControllerGetAxis(controller,SDL_CONTROLLER_AXIS_LEFTX);
    const auto y = SDL_GameControllerGetAxis(controller,SDL_CONTROLLER_AXIS_LEFTY);

    // input of more than half in either direction is enough to make
    // to cause an input
    constexpr int16_t threshold = std::numeric_limits<int16_t>::max() / 2;


    // if something is greater than threshold and not pushed before
    // key press or if it was and now isnt release the key
    // do for all 4 keys


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
            gba.button_event(button::down,false);
        }

        if(prev_dpad[UP])
        {
            gba.button_event(button::up,false);
        }
        prev_dpad[LEFT] = false;
        prev_dpad[RIGHT] = false;
    }

    // in x axis deadzone deset both
    if(x == threshold)
    {
        if(prev_dpad[LEFT])
        {
            gba.button_event(button::left,false);
        }

        if(prev_dpad[RIGHT])
        {
            gba.button_event(button::right,false);
        }
        prev_dpad[LEFT] = false;
        prev_dpad[RIGHT] = false;
    }


    const bool r = x > threshold;
    const bool l = x < -threshold;
    const bool u = y < -threshold;
    const bool d = y > threshold;


    // right
    if(prev_dpad[RIGHT] != r)
    {
        gba.button_event(button::right,r);
        prev_dpad[RIGHT] = r;
    }

    // left
    if(prev_dpad[LEFT] != l)
    {
        gba.button_event(button::left,l);
        prev_dpad[LEFT] = l;
    }

    // up
    if(prev_dpad[UP] != u)
    {
        gba.button_event(button::up,u);
        prev_dpad[UP] = u;    
    }

    // down
    if(prev_dpad[DOWN] != d)
    {
        gba.button_event(button::down,d);
        prev_dpad[DOWN] = d;    
    }

    // handle analog triggers
    const auto trig_l = SDL_GameControllerGetAxis(controller,SDL_CONTROLLER_AXIS_TRIGGERLEFT);
    const auto trig_r = SDL_GameControllerGetAxis(controller,SDL_CONTROLLER_AXIS_TRIGGERRIGHT);

    static bool trig_l_prev = false;
    static bool trig_r_prev = false;

    const bool trig_l_cur = trig_l > threshold;
    const bool trig_r_cur = trig_r > threshold;

    if(trig_l_prev != trig_l_cur)
    {
        gba.button_event(button::l,trig_l_cur);
        trig_l_prev = trig_l_cur;
    }

    if(trig_r_prev != trig_r_cur)
    {
        gba.button_event(button::r,trig_r_cur);
        trig_r_prev = trig_r_cur;
    }

}

GbaControllerInput::~GbaControllerInput()
{
    if(controller_connected && controller != NULL)
    {
        // this is apparently buggy...
        //SDL_GameControllerClose(controller);
        controller = NULL;
    }
}

#endif