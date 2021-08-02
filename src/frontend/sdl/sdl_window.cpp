#ifdef FRONTEND_SDL
#include <frontend/sdl/sdl_window.h>
#include <destoer-emu/destoer-emu.h>


SDLMainWindow::SDLMainWindow(std::string filename)
{
	try
	{
		const auto type = get_emulator_type(filename);
	

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

			case emu_type::n64:
			{
				puts("not supported");
				break;
			}

			case emu_type::none:
			{
				std::cout << "unrecognised rom type" 
					<< filename << "\n";
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
	SDL_GL_SetSwapInterval(1);

	//constexpr uint32_t fps = 60; 
	//constexpr uint32_t screen_ticks_per_frame = 1000 / fps;
	//uint64_t next_time = current_time() + screen_ticks_per_frame;
    gb.reset(filename);
    init_sdl(gameboy::SCREEN_WIDTH,gameboy::SCREEN_HEIGHT);


	FpsCounter fps_counter;


	/* setup our controller */
	GbControllerInput controller;
	controller.init();

    for(;;)
    {
		fps_counter.reading_start();

        gameboy_handle_input(controller);

		controller.update(gb);

        gb.run();

        gameboy_render();

		// throttle the emulation
		if(gb.throttle_emu)
		{
        	//SDL_Delay(time_left(next_time));
			SDL_GL_SetSwapInterval(1);
		}

		else
		{
			//SDL_Delay(time_left(next_time) / 8);
			SDL_GL_SetSwapInterval(0);
		}

		fps_counter.reading_end();

		SDL_SetWindowTitle(window,fmt::format("destoer-emu: {}",fps_counter.get_fps()).c_str());

		//next_time = current_time() + screen_ticks_per_frame;
		
		// we hit a breakpoint go back to the prompt
	#ifdef DEBUG
		if(gb.debug.is_halted())
		{
			gb.debug.debug_input();
		}
	#endif
    }	
}

void SDLMainWindow::gameboy_handle_input(GbControllerInput &controller)
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
			#ifdef DEBUG
				if(event.key.keysym.sym == SDLK_p)
				{
					gb.debug.debug_input();
				}

				else
			#endif
				{
					gb.key_input(event.key.keysym.sym,true);
				}
				break;
			}
			
			case SDL_KEYUP:
			{
				gb.key_input(event.key.keysym.sym,false);
				break;
			}

			case SDL_CONTROLLERDEVICEADDED: controller.connected(event.cdevice.which); break;
			case SDL_CONTROLLERDEVICEREMOVED: controller.disconnected(event.cdevice.which); break;

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

void SDLMainWindow::gba_handle_input(GbaControllerInput &controller)
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
				gba.mem.save_cart_ram();
                puts("quitting...");
                exit(1);
			}	
			
			case SDL_KEYDOWN:
			{
			#ifdef DEBUG
				if(event.key.keysym.sym == SDLK_p)
				{
					gba.debug.debug_input();
				}

				else
			#endif
				{
					gba.key_input(event.key.keysym.sym,true);
				}
                break;
			}
			
			case SDL_KEYUP:
			{
				gba.key_input(event.key.keysym.sym,false);
                break;
			}

			case SDL_CONTROLLERDEVICEADDED: controller.connected(event.cdevice.which); break;
			case SDL_CONTROLLERDEVICEREMOVED: controller.disconnected(event.cdevice.which); break;


            default:
            {
                break;
            }           
		}
	}
}

void SDLMainWindow::gba_main(std::string filename)
{
	SDL_GL_SetSwapInterval(1);

	//constexpr uint32_t fps = 60; 
	//constexpr uint32_t screen_ticks_per_frame = 1000 / fps;
	//uint64_t next_time = current_time() + screen_ticks_per_frame;
    gba.reset(filename);
    init_sdl(gameboyadvance::SCREEN_WIDTH,gameboyadvance::SCREEN_HEIGHT);

	FpsCounter fps_counter;

	/* setup our controller */
	GbaControllerInput controller;
	controller.init();

	for(;;)
	{
		fps_counter.reading_start();

		gba_handle_input(controller);

		controller.update(gba);

		gba.run();
		gba_render();

		// throttle the emulation
		if(gba.throttle_emu)
		{
        	//SDL_Delay(time_left(next_time));
			SDL_GL_SetSwapInterval(1);
		}

		else
		{
			//SDL_Delay(time_left(next_time) / 8);
			SDL_GL_SetSwapInterval(0);
		}

		fps_counter.reading_end();

		SDL_SetWindowTitle(window,fmt::format("destoer-emu: {}",fps_counter.get_fps()).c_str());

		//next_time = current_time() + screen_ticks_per_frame;
	#ifdef DEBUG
		if(gba.debug.is_halted())
		{
			gba.debug.debug_input();
		}
	#endif
	}
}

SDLMainWindow::~SDLMainWindow()
{
	if(renderer)
	{
    	SDL_DestroyRenderer(renderer);
	}

	if(window)
	{
    	SDL_DestroyWindow(window);
	}

	if(texture)
	{
		SDL_DestroyTexture(texture);
	}

    SDL_QuitSubSystem(SDL_INIT_EVERYTHING);  
}
#endif