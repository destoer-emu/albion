#include <destoer.cpp>
#include <frontend/qt/qt_window.h>
#include <frontend/sdl/sdl_window.h>
#include <frontend/imgui/imgui_window.h>
#include <frontend/destoer/destoer_window.h>
#include <albion/lib.h>
#include <gba/gba.h>
#include <gb/gb.h>

#ifdef SDL_REQUIRED
#define SDL_MAIN_HANDLED
#ifdef _WIN32
#include <SDL.H>
#else
#include <SDL2/SDL.h>
#endif
#endif

#ifndef FRONTEND_HEADLESS 

// gameboy test running
void gb_run_test_helper(const std::vector<std::string> &tests, int seconds)
{

    int fail = 0;
    int pass = 0;
    int aborted = 0;
    int timeout = 0;

    gameboy::GB gb;

    for(const auto &x: tests)
    {
        
        try
        {
            gb.reset(x);
            gb.apu.playback.stop();
            gb.throttle_emu = false;


            auto start = std::chrono::system_clock::now();

            // add a timer timeout if required
            for(;;)
            {
                gb.run();


                if(gb.mem.test_result == emu_test::fail)
                {
                    std::cout << fmt::format("{}: fail\n",x);
                    fail++;
                    break;
                }

                else if(gb.mem.test_result == emu_test::pass)
                {
                    // we are passing so many compared to fails at this point
                    // it doesnt make sense to print them
                    //std::cout << fmt::format("{}: pass\n",x);
                    pass++;
                    break;
                }

                auto current = std::chrono::system_clock::now();

                // if the test takes longer than 5 seconds time it out
                if(std::chrono::duration_cast<std::chrono::seconds>(current - start).count() > seconds)
                {
                    std::cout << fmt::format("{}: timeout\n",x);
                    timeout++;
                    break;
                }
            }
        }

        catch(std::exception &ex)
        {
            std::cout << fmt::format("{}: aborted {}\n",x,ex.what());
            aborted++;
        }
    }

    printf("total: %zd\n",tests.size());
    printf("pass: %d\n",pass);
    printf("fail: %d\n",fail);
    printf("abort: %d\n",aborted);
    printf("timeout: %d\n",timeout);    
}

void gb_run_tests()
{
    puts("gekkio_tests:");
    auto start = std::chrono::system_clock::now();
    const auto [tree,error] = read_dir_tree("mooneye-gb_hwtests");
    gb_run_test_helper(filter_ext(tree,"gb"),10);
    auto current = std::chrono::system_clock::now();
    auto count = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(current - start).count()) / 1000.0;
    printf("total time taken %f\n",count);
}

#endif

int main(int argc, char *argv[])
{  
    UNUSED(argc); UNUSED(argv);
#ifndef FRONTEND_HEADLESS    
    if(argc == 2)
    {
        if(std::string(argv[1]) == "-t")
        {
            try
            {
                gb_run_tests();
            }

            catch(std::exception &ex)
            {
                std::cout << ex.what();
            }

            return 0;
        }
    }
#endif

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


#ifdef FRONTEND_QT 
    QApplication app(argc, argv);

    QtMainWindow window;

    window.setWindowTitle("albion: no rom");
    window.show();

    return app.exec();
#endif

#ifdef FRONTEND_SDL
    
    if(argc != 2)
    {
        printf("usage: %s <rom_name>\n",argv[0]);
        return 0;
    }
    SDLMainWindow window(argv[1]);
#endif

#ifdef FRONTEND_IMGUI
    UNUSED(argc); UNUSED(argv);

    ImguiMainWindow window;
    std::string rom_name = "";
    if(argc == 2)
    {
        rom_name = argv[1];
    }

    window.mainloop(rom_name);
#endif

#ifdef FRONTEND_DESTOER
    destoer_ui();
#endif

#ifdef SDL_REQUIRED
    SDL_Quit();
#endif

    return 0;
}
