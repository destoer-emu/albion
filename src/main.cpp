#include <destoer.cpp>
#include <frontend/sdl/sdl_window.h>
#include <frontend/imgui/imgui_window.h>
#include <frontend/destoer/destoer_window.h>
#include <albion/lib.h>

#ifdef SDL_REQUIRED
#define SDL_MAIN_HANDLED
#ifdef _WIN32
#include <SDL.H>
#include <cfenv>

#else
#include <SDL2/SDL.h>
#endif
#endif

#include "test.cpp"
#include "spdlog/spdlog.h"

int main(int argc, char *argv[])
{  
    UNUSED(argc); UNUSED(argv);
#ifndef FRONTEND_HEADLESS    
    if(argc == 2)
    {
        std::string arg(argv[1]);
        if(arg == "-t")
        {
            try
            {
                run_tests();
            }

            catch(std::exception &ex)
            {
                std::cout << ex.what();
            }

            return 0;
        }

        if (arg == "-dev-gen")
        {
        }
    }
#endif

    spdlog::set_pattern("[%H:%M:%S.%e] [%l] %v");
    std::fesetround(FE_TONEAREST);

// if sdl is used for anything we need to init it here
#ifdef SDL_REQUIRED
    // sdl required for audio
    SDL_SetMainReady();
	if(SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		printf("Unable to initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}
#endif

#ifdef FRONTEND_SDL
    
    Config cfg = get_config(argc,argv);

    if(argc < 2)
    {
        printf("usage: %s <rom_name>\n",argv[0]);
        return 0;
    }
    start_emu(argv[1],cfg);
#endif

#ifdef FRONTEND_IMGUI
    UNUSED(argc); UNUSED(argv);

    std::string rom_name = "";
    if(argc == 2)
    {
        rom_name = argv[1];
    }

    mainloop(rom_name);
#endif

#ifdef FRONTEND_DESTOER
    destoer_ui();
#endif

#ifdef SDL_REQUIRED
    SDL_Quit();
#endif

    return 0;
}
