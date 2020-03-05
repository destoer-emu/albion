#ifdef FRONTEND_SDL
#include "sdl_window.h"
#include <destoer-emu/emulator.h>


SDLMainWindow::SDLMainWindow(std::string filename)
{
	emu_type type;
	try
	{
		type = get_emulator_type(filename);
	

		switch(type)
		{
			case emu_type::gameboy:
			{
				gameboy_main(filename);
				break;
			}

			case emu_type::gba:
			{
				gba_main(filename);
				break;
			}
		}
	}

	catch(std::exception &ex)
	{
		std::cout << ex.what() << "\n";
		return;
	}
}


void SDLMainWindow::gameboy_main(std::string filename)
{
	constexpr uint32_t fps = 60; 
	constexpr uint32_t screen_ticks_per_frame = 1000 / fps;
	uint64_t next_time = current_time() + screen_ticks_per_frame;
    gb.reset(filename);
    init_sdl(gameboy::SCREEN_WIDTH,gameboy::SCREEN_HEIGHT);

    for(;;)
    {
        gameboy_handle_input();

        gb.run();

        gameboy_render();

		// throttle the emulation
		if(gb.throttle_emu)
		{
        	SDL_Delay(time_left(next_time));
		}

		else
		{
			SDL_Delay(time_left(next_time) / 8);
		}
		next_time = current_time() + screen_ticks_per_frame;
    }	
}

void SDLMainWindow::gameboy_handle_input()
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
				gb.key_input(event.key.keysym.sym,true);
				break;
			}
			
			case SDL_KEYUP:
			{
				gb.key_input(event.key.keysym.sym,false);
				break;
			}

	
			case SDL_QUIT:
			{
				gb.mem.save_cart_ram();
				exit(0);
			}
        }
    }    
}

void SDLMainWindow::init_sdl(int x, int y)
{
	X = x;
	Y = y;

	if(SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		std::string err = fmt::format("Unable to initialize SDL: {}", SDL_GetError());
		throw std::runtime_error(err);
	}

	// initialize our window
	window = SDL_CreateWindow("destoer-emu",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,X*2,Y*2,SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
	
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl"); // crashes without this on windows?
	
	// set a render for our window
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	texture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_BGR888, SDL_TEXTUREACCESS_STREAMING, X, Y);
	std::fill(gb.ppu.screen.begin(),gb.ppu.screen.end(),255);	    
}


void SDLMainWindow::gameboy_render()
{
    // do our screen blit
    SDL_UpdateTexture(texture, NULL, gb.ppu.screen.data(),  4 * X);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);    
}

void SDLMainWindow::gba_render()
{
    // do our screen blit
    SDL_UpdateTexture(texture, NULL, gba.disp.screen.data(),  4 * X);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);    
}

void SDLMainWindow::gba_handle_input()
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
		
	
			case SDL_QUIT:
			{
                puts("quitting...");
                exit(1);
			}	
			
			case SDL_KEYDOWN:
			{
				gba.key_input(event.key.keysym.sym,true);
                break;
			}
			
			case SDL_KEYUP:
			{
				gba.key_input(event.key.keysym.sym,false);
                break;
			}

            default:
            {
                break;
            }           
		}
	}
}

void SDLMainWindow::gba_main(std::string filename)
{
	constexpr uint32_t fps = 60; 
	constexpr uint32_t screen_ticks_per_frame = 1000 / fps;
	uint64_t next_time = current_time() + screen_ticks_per_frame;
    gba.reset(filename);
    init_sdl(gameboyadvance::SCREEN_WIDTH,gameboyadvance::SCREEN_HEIGHT);

	for(;;)
	{
		gba_handle_input();
		gba.run();
		gba_render();

		SDL_Delay(time_left(next_time));
		next_time = current_time() + screen_ticks_per_frame;
	}
}

SDLMainWindow::~SDLMainWindow()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_EVERYTHING);
    SDL_Quit();    
}
#endif