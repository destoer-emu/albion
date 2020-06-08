#include <frontend/qt/qt_window.h>
#include <frontend/sdl/sdl_window.h>
#include <frontend/imgui/imgui_window.h>


#define SDL_MAIN_HANDLED
#ifdef _WIN32
#include <SDL.H>
#else
#include <SDL2/SDL.h>
#endif

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
                    std::cout << fmt::format("{}: pass\n",x);
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
    gb_run_test_helper(filter_ext(get_dir_tree("mooneye-gb_hwtests"),"gb"),10);
    auto current = std::chrono::system_clock::now();
    auto count = std::chrono::duration_cast<std::chrono::seconds>(current - start).count();
    printf("total time taken %ld\n",count);
}

int main(int argc, char *argv[])
{  
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

    window.setWindowTitle("destoer-emu: no rom");
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
    window.mainloop();
#endif

#ifdef SDL_REQUIRED
    SDL_Quit();
#endif

    return 0;
}
