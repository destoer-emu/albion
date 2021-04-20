#include <frontend/controller.h>
#include <destoer-emu/lib.h>

#ifdef CONTROLLER_SDL
void Controller::init()
{
    // pick first valid controller
	for(int i = 0; i < SDL_NumJoysticks(); i++)
	{
		if(SDL_IsGameController(i))
		{
			controller = SDL_GameControllerOpen(i);
            if(controller == NULL)
            {
                throw std::runtime_error("could not open controller!");
            }
            controller_connected = true;
            this->id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));
            break;
		}
	}  
}

void Controller::connected(int id)
{
    if(SDL_IsGameController(id) && !controller_connected)
    {
        controller = SDL_GameControllerOpen(id);
        if(controller == NULL)
        {
            throw std::runtime_error("could not open controller!");
        }
        controller_connected = true;
        // just sdl things
        this->id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));
    }    
}

void Controller::disconnected(int id)
{
    if(this->id == id)
    {
        controller = NULL;
        controller_connected = false;
        this->id = -1;
    }
}

Controller::~Controller()
{
    if(controller_connected && controller != NULL)
    {
        // this is apparently buggy...
        //SDL_GameControllerClose(controller);
        controller = NULL;
    }
}
#endif