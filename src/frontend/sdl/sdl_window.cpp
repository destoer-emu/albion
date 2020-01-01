#ifdef FRONTEND_SDL
#include "sdl_window.h"


SDLMainWindow::SDLMainWindow(std::string file_name)
{
	constexpr uint32_t fps = 60; 
	constexpr uint32_t screen_ticks_per_frame = 1000 / fps;
	uint64_t next_time = current_time() + screen_ticks_per_frame;
    gb.reset(file_name);
    init_sdl();

    for(;;)
    {
        handle_input();

        gb.run();

        render();

		// throttle the emulation
        SDL_Delay(time_left(next_time));
		next_time += screen_ticks_per_frame;
    }
}


void SDLMainWindow::handle_input()
{
	SDL_Event event;
	
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

			case SDL_KEYDOWN:
			{
				switch(event.key.keysym.sym) // <--- could remove as repeated code
				{
					case SDLK_a: gb.key_pressed(4); break;
					case SDLK_s: gb.key_pressed(5); break;
					case SDLK_RETURN: gb.key_pressed(7); break;
					case SDLK_SPACE: gb.key_pressed(6); break;
					case SDLK_RIGHT: gb.key_pressed(0); break;
					case SDLK_LEFT: gb.key_pressed(1); break;
					case SDLK_UP: gb.key_pressed(2);break;
					case SDLK_DOWN: gb.key_pressed(3); break;
				}
				break;
			}
			
			case SDL_KEYUP:
			{
				switch(event.key.keysym.sym)
				{
					case SDLK_a: gb.key_released(4); break;
					case SDLK_s: gb.key_released(5); break;
					case SDLK_RETURN: gb.key_released(7); break;
					case SDLK_SPACE: gb.key_released(6); break;
					case SDLK_RIGHT: gb.key_released(0); break;
					case SDLK_LEFT: gb.key_released(1); break;
					case SDLK_UP: gb.key_released(2); break;
					case SDLK_DOWN: gb.key_released(3);break;
				}
				break;
			}

	
			case SDL_QUIT:
			{
				exit(0);
			}
        }
    }    
}

void SDLMainWindow::init_sdl()
{
	if(SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		std::string err = fmt::format("Unable to initialize SDL: {}", SDL_GetError());
		throw std::runtime_error(err);
	}

	// initialize our window
	window = SDL_CreateWindow("destoer-emu",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,gb.ppu.X*2,gb.ppu.Y*2,SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
	
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl"); // crashes without this on windows?
	
	// set a render for our window
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	texture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, gb.ppu.X, gb.ppu.Y);
	std::fill(gb.ppu.screen.begin(),gb.ppu.screen.end(),255);	    
}


void SDLMainWindow::render()
{
    // do our screen blit
    SDL_UpdateTexture(texture, NULL, gb.ppu.screen.data(),  4 * gb.ppu.X);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);    
}

SDLMainWindow::~SDLMainWindow()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_EVERYTHING);
    SDL_Quit();    
}
#endif