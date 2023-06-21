#include "n64_window.h"


void N64Window::init(const std::string& filename)
{
    init_sdl(320,240);
    input.init();
    reset(n64,filename);	
}

void N64Window::pass_input_to_core()
{

    input.controller.input_events.clear();
}

void N64Window::core_quit()
{
    //n64.mem.save_cart_ram();
    exit(0);   
}

void N64Window::run_frame()
{
    run(n64);
    render(n64.rdp.screen.data());
}

void N64Window::debug_halt()
{
    n64.debug.debug_input();
}

void N64Window::core_throttle()
{

}

void N64Window::core_unbound()
{

}

void N64Window::handle_debug()
{
    if(n64.debug.is_halted())
    {
        n64.debug.debug_input();
    }
}