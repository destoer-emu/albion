#include <destoer.h>
#include <albion/lib.h>
#ifndef FRONTEND_HEADLESS 

#ifdef GB_ENABLED
#include <gb/gb.h>

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

void run_tests()
{
#ifdef GB_ENABLED
    gb_run_tests();    
#endif
}

#endif