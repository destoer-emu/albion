#include <frontend/qt/qt_window.h>
#include <frontend/sdl/sdl_window.h>
#include <frontend/imgui/imgui_window.h>

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

    printf("total: %d\n",tests.size());
    printf("pass: %d\n",pass);
    printf("fail: %d\n",fail);
    printf("abort: %d\n",aborted);
    printf("timeout: %d\n",timeout);    
}

void gb_run_tests()
{
    puts("gekkio_tests:");
    gb_run_test_helper(filter_ext(get_dir_tree("mooneye-gb_hwtests"),"gb"),10);
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

    SDL_SetMainReady();
    SDLMainWindow window(argv[1]);
#endif

#ifdef FRONTEND_IMGUI
    UNUSED(argc); UNUSED(argv);

    ImguiMainWindow window;
    window.mainloop();
#endif

    return 0;
}
