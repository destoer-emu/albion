#ifdef FRONTEND_SDL
#include "sdl_window.h"


SDLMainWindow::SDLMainWindow(std::string filename)
{
	//gameboy_main(filename);
	gba_main(filename);
}


void SDLMainWindow::gameboy_main(std::string filename)
{
	constexpr uint32_t fps = 60; 
	constexpr uint32_t screen_ticks_per_frame = 1000 / fps;
	uint64_t next_time = current_time() + screen_ticks_per_frame;
    gb.reset(filename);
    init_sdl(gb.ppu.X,gb.ppu.Y);

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

					case SDLK_KP_PLUS:
					{
						gb.apu.stop_audio();
						gb.throttle_emu = false;
						break;
					}

					case SDLK_KP_MINUS:
					{
						gb.apu.start_audio();
						gb.throttle_emu = true;						
						break;
					}

				}
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
				switch(event.key.keysym.sym)
				{

					case SDLK_RETURN:
					{
						gba.button_event(Button::START,true);
						break;						
					}

					case SDLK_SPACE:
					{
						gba.button_event(Button::SELECT,true);
						break;
					}

					case SDLK_DOWN:
					{
						gba.button_event(Button::DOWN,true);
						break;
					}

					case SDLK_UP:
					{
						gba.button_event(Button::UP,true);
						break;
					}

					case SDLK_LEFT:
					{
						gba.button_event(Button::LEFT,true);
						break;
					}

					case SDLK_RIGHT:
					{
						gba.button_event(Button::RIGHT,true);
						break;
					}


					case SDLK_a:
					{
						gba.button_event(Button::A,true);
						break;
					}

					case SDLK_s:
					{
						gba.button_event(Button::B,true);
						break;
					}



                    default:
                    {
                        break;
                    }
                }
                break;
			}
			
			case SDL_KEYUP:
			{
				switch(event.key.keysym.sym)
				{

					case SDLK_RETURN:
					{
						gba.button_event(Button::START,false);
						break;						
					}

					case SDLK_SPACE:
					{
						gba.button_event(Button::SELECT,false);
						break;
					}

					case SDLK_DOWN:
					{
						gba.button_event(Button::DOWN,false);
						break;
					}

					case SDLK_UP:
					{
						gba.button_event(Button::UP,false);
						break;
					}

					case SDLK_LEFT:
					{
						gba.button_event(Button::LEFT,false);
						break;
					}

					case SDLK_RIGHT:
					{
						gba.button_event(Button::RIGHT,false);
						break;
					}


					case SDLK_a:
					{
						gba.button_event(Button::A,false);
						break;
					}

					case SDLK_s:
					{
						gba.button_event(Button::B,false);
						break;
					}


                    default:
                    {
                        break;
                    }    
                }
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
    init_sdl(gba.disp.X,gba.disp.Y);

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