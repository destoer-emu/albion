#include "imgui_window.h"
#include <n64/n64.h>

void ImguiMainWindow::n64_stop_instance()
{
    n64.quit = true;
    emu_running = false;
}

void ImguiMainWindow::n64_start_instance()
{
    n64.quit = false;
    emu_running = true;
    n64.debug.wake_up();
}

void ImguiMainWindow::n64_new_instance(std::string filename, bool use_bios)
{
    try
    {
        n64_reset_instance(filename,use_bios);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return; 
    }

    n64_start_instance();  
}

void ImguiMainWindow::n64_reset_instance(std::string filename, bool use_bios)
{
    UNUSED(use_bios);
    reset(n64,filename);
}

void ImguiMainWindow::n64_run_frame()
{
    try
    {
        nintendo64::run(n64);
    }
    

    catch(std::exception &ex)
    {
        n64.debug.write_logger(ex.what());
        std::cout << ex.what() << "\n";
        emu_running = false;
        SDL_GL_SetSwapInterval(1); // Enable vsync
        return;
    }
}