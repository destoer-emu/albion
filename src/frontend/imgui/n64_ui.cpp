#include "n64_ui.h"
using namespace nintendo64;

void N64Window::start_instance()
{
    emu_running = true;
    n64.debug.wake_up();    
}

void N64Window::reset_instance(const std::string& name, b32 use_bios)
{
    UNUSED(use_bios);
    screen.init_texture(640,480);
    reset(n64,name);
}

void N64Window::stop_instance()
{
    n64.quit = true;
    emu_running = false;    
}


void N64Window::run_frame()
{
    n64.debug.log_enabled = log_enabled;

    try
    {
        //handle_input(n64,input.controller);
  
        run(n64);

        if(n64.rdp.frame_done)
        {
            // swap the buffer so the frontend can render it
            screen.swap_buffer(n64.rdp.screen);
        }
    }

    catch(std::exception &ex)
    {
        std::cout << ex.what() << "\n";
        emu_running = false;
        SDL_GL_SetSwapInterval(1); // Enable vsync
        return;
    }    
}