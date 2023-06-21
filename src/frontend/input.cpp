#include <frontend/input.h>
#include <albion/lib.h>

#ifdef CONTROLLER_SDL
void Input::init()
{
    // pick first valid controller
	for(int i = 0; i < SDL_NumJoysticks(); i++)
	{
		if(SDL_IsGameController(i))
		{
			game_controller = SDL_GameControllerOpen(i);
            if(game_controller == NULL)
            {
                throw std::runtime_error("could not open controller!");
            }
            controller_connected = true;
            this->id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(game_controller));
            break;
		}
	}  
}

void Input::connect_controller(int id)
{
    if(SDL_IsGameController(id) && !controller_connected)
    {
        game_controller = SDL_GameControllerOpen(id);
        if(game_controller == NULL)
        {
            throw std::runtime_error("could not open controller!");
        }
        controller_connected = true;
        // just sdl things
        this->id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(game_controller));
    }    
}

void Input::disconnect_controller(int id)
{
    if(this->id == id)
    {
        game_controller = NULL;
        controller_connected = false;
        this->id = -1;
    }
}

Input::~Input()
{
    if(controller_connected && game_controller != NULL)
    {
        // this is apparently buggy...
        //SDL_GameControllerClose(controller);
        game_controller = NULL;
    }
}


void Input::add_event_from_key(s32 key, b32 down)
{
	switch(key)
	{
		case SDLK_RETURN: controller.add_event(controller_input::start,down); break;
		case SDLK_SPACE: controller.add_event(controller_input::select,down); break;


		case SDLK_UP: controller.add_event(controller_input::up,down); break;
		case SDLK_DOWN: controller.add_event(controller_input::down,down); break;
		case SDLK_LEFT: controller.add_event(controller_input::left,down); break;
		case SDLK_RIGHT: controller.add_event(controller_input::right,down); break;

		case SDLK_a: controller.add_event(controller_input::a,down); break;
		case SDLK_s: controller.add_event(controller_input::x,down); break;
		case SDLK_d: controller.add_event(controller_input::left_trigger,down); break;
		case SDLK_f: controller.add_event(controller_input::right_trigger,down); break;

		default: break;
	}
}

/*
enum class emu_control
{
    debug_t,
    throttle,
    unbound_t,
    break_t,
    quit_t,
    none,
}

*/


emu_control Input::handle_input(SDL_Window* window)
{
    b32 key_pressed = false;

	SDL_Event event;
	
	emu_control control = emu_control::none_t;

	// handle input
	while(SDL_PollEvent(&event))
	{	
		switch(event.type) 
		{
			case SDL_WINDOWEVENT:
			{
				if(event.window.event == SDL_WINDOWEVENT_RESIZED)
				{
					SDL_SetWindowSize(window,event.window.data1, event.window.data2);
				}
				break;
			}
		
	
			case SDL_QUIT:
			{
				control = emu_control::quit_t;
				break;
			}	
			
			case SDL_KEYDOWN:
			{
				key_pressed = true;


				const s32 key = event.key.keysym.sym;

				switch(key)
				{
					case SDLK_p:
					{
						control = emu_control::break_t;
						break;
					}

					case SDLK_k:
					{
						control = emu_control::unbound_t;
						break;
					}

					case SDLK_l:
					{
						control = emu_control::throttle_t;
						break;
					}

					default:
					{
						add_event_from_key(event.key.keysym.sym,true);
						break;
					}
				}
                break;
			}
			
			case SDL_KEYUP:
			{
				add_event_from_key(event.key.keysym.sym,false);
                break;
			}

			case SDL_CONTROLLERDEVICEADDED: connect_controller(event.cdevice.which); break;
			case SDL_CONTROLLERDEVICEREMOVED: disconnect_controller(event.cdevice.which); break;


            default:
            {
                break;
            }  
        }
    }

	// no keys pressed handle controller events
	if(!key_pressed)
	{
		handle_controller_input();
	}

	return control;
}

void Input::handle_controller_input()
{
    // no controller we dont care
    if(!controller_connected)
    {
        return;
    }

    // get game controller update
    SDL_GameControllerUpdate();

    static constexpr SDL_GameControllerButton sdl_buttons[] = 
    {
        SDL_CONTROLLER_BUTTON_A,
        SDL_CONTROLLER_BUTTON_X,
        SDL_CONTROLLER_BUTTON_START,
        SDL_CONTROLLER_BUTTON_BACK,
        SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
        SDL_CONTROLLER_BUTTON_LEFTSHOULDER
    };

    static constexpr controller_input controller_buttons[] = 
    {
        controller_input::a,
        controller_input::x,
        controller_input::start,
        controller_input::select,
        controller_input::right_trigger,
        controller_input::left_trigger,
    };
    static_assert(sizeof(controller_buttons) == sizeof(sdl_buttons));


    static constexpr int CONTROLLER_BUTTONS_SIZE = sizeof(controller_buttons) / sizeof(controller_buttons[0]);

    // cache the old state so we know when a new key press has occured
    static bool buttons_prev[CONTROLLER_BUTTONS_SIZE] = {false};

    // check the controller button state
    for(int i = 0; i < CONTROLLER_BUTTONS_SIZE; i++)
    {
        // now we handle controller inputs
        auto b = SDL_GameControllerGetButton(game_controller,sdl_buttons[i]);
        if(b && !buttons_prev[i])
        {
			controller.add_event(controller_buttons[i],true);
        }

        else if(!b && buttons_prev[i])
        {
			controller.add_event(controller_buttons[i],false);
        }
        buttons_prev[i] = b;
    }


    // handle the joystick
    const auto x = SDL_GameControllerGetAxis(game_controller,SDL_CONTROLLER_AXIS_LEFTX);
    const auto y = SDL_GameControllerGetAxis(game_controller,SDL_CONTROLLER_AXIS_LEFTY);

    // input of more than half in either direction is enough to make
    // to cause an input
    constexpr int16_t threshold = std::numeric_limits<int16_t>::max() / 2;


	// NOTE: this is effectively a digital input for the anlog stick and should be ignored by the handler when we
	// actually want an input the stick directly

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
			controller.add_event(controller_input::down,false);
        }

        if(prev_dpad[UP])
        {
			controller.add_event(controller_input::up,false);
        }
        prev_dpad[LEFT] = false;
        prev_dpad[RIGHT] = false;
    }

    // in x axis deadzone deset both
    if(x == threshold)
    {
        if(prev_dpad[LEFT])
        {
			controller.add_event(controller_input::left,false);
        }

        if(prev_dpad[RIGHT])
        {
			controller.add_event(controller_input::right,false);
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
		controller.add_event(controller_input::right,r);
        prev_dpad[RIGHT] = r;
    }

    // left
    if(prev_dpad[LEFT] != l)
    {
		controller.add_event(controller_input::left,l);
        prev_dpad[LEFT] = l;
    }

    // up
    if(prev_dpad[UP] != u)
    {
		controller.add_event(controller_input::up,u);
        prev_dpad[UP] = u;    
    }

    // down
    if(prev_dpad[DOWN] != d)
    {
		controller.add_event(controller_input::down,d);
        prev_dpad[DOWN] = d;    
    }

    // handle analog triggers
    const auto trig_l = SDL_GameControllerGetAxis(game_controller,SDL_CONTROLLER_AXIS_TRIGGERLEFT);
    const auto trig_r = SDL_GameControllerGetAxis(game_controller,SDL_CONTROLLER_AXIS_TRIGGERRIGHT);

    static bool trig_l_prev = false;
    static bool trig_r_prev = false;

    const bool trig_l_cur = trig_l > threshold;
    const bool trig_r_cur = trig_r > threshold;

    if(trig_l_prev != trig_l_cur)
    {
		controller.add_event(controller_input::left_trigger,trig_l_cur);
        trig_l_prev = trig_l_cur;
    }

    if(trig_r_prev != trig_r_cur)
    {
		controller.add_event(controller_input::right_trigger,trig_r_cur);
        trig_r_prev = trig_r_cur;
    }

}


#endif