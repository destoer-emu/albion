#ifdef FRONTEND_SDL
#include <frontend/sdl/sdl_window.h>

#ifdef GB_ENABLED
#include <frontend/sdl/gb_window.h>
#include "gb_window.cpp"
#endif

#ifdef GBA_ENABLED
#include <frontend/sdl/gba_window.h>
#include "gba_window.cpp"
#endif

#ifdef N64_ENABLED
#include <frontend/sdl/n64_window.h>
#include "n64_window.cpp"
#endif

#include <albion/destoer-emu.h>

void start_emu(std::string filename)
{
	try
	{
		const auto type = get_emulator_type(filename);
	

		switch(type)
		{
		#ifdef GB_ENABLED
			case emu_type::gameboy:
			{
				GameboyWindow gb;
				gb.main(filename);
				break;
			}
		#endif

		#ifdef GBA_ENABLED
			case emu_type::gba:
			{
				GBAWindow gba;
				gba.main(filename);
				break;
			}
		#endif

		#ifdef N64_ENABLED
			case emu_type::n64:
			{
				N64Window n64;
				n64.main(filename);
				break;
			}
		#endif

			default:
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


void SDLMainWindow::create_texture(u32 x, u32 y)
{
	// destroy the old one if need be
	if(texture)
	{
		SDL_DestroyTexture(texture);
	}

	X = x;
	Y = y;

	texture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, x, y);


	// resize the window
	SDL_SetWindowSize(window,x * 2, y * 2);
}

void SDLMainWindow::init_sdl(u32 x, u32 y)
{
	// initialize our window
	window = SDL_CreateWindow("albion",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,x * 2,y *2,SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
	
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl"); // crashes without this on windows?
	
	// set a render for our window
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	create_texture(x,y);
	SDL_GL_SetSwapInterval(1);
}

void SDLMainWindow::render(const u32* data)
{
    // do our screen blit
    SDL_UpdateTexture(texture, NULL, data,  4 * X);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);    	
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


void SDLMainWindow::main(std::string filename)
{
	SDL_GL_SetSwapInterval(1);

	//constexpr uint32_t fps = 60; 
	//constexpr uint32_t screen_ticks_per_frame = 1000 / fps;
	//uint64_t next_time = current_time() + screen_ticks_per_frame;
	init(filename);

	FpsCounter fps_counter;


    for(;;)
    {
		fps_counter.reading_start();
		auto control = input.handle_input(window);
		
		pass_input_to_core();

		run_frame();

		switch(control)
		{
			case emu_control::quit_t:
			{
				core_quit();
				break;
			}

			case emu_control::throttle_t:
			{
				//SDL_Delay(time_left(next_time));
				SDL_GL_SetSwapInterval(1);
				core_throttle();
				break;
			}

			case emu_control::unbound_t:
			{
				//SDL_Delay(time_left(next_time) / 8);
				SDL_GL_SetSwapInterval(0);
				core_unbound();
				break;
			}

			case emu_control::break_t:
			{
				debug_halt();
				break;
			}

			case emu_control::none_t: break;
		}

		fps_counter.reading_end();

		SDL_SetWindowTitle(window,std::format("albion: {}",fps_counter.get_fps()).c_str());

		//next_time = current_time() + screen_ticks_per_frame;
		
		// we hit a breakpoint go back to the prompt
		handle_debug();
    }	
}

#endif