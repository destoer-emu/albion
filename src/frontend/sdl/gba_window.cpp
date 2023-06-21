#include "gba_window.h"

void GBAWindow::init(const std::string& filename)
{
    init_sdl(gameboyadvance::SCREEN_WIDTH,gameboyadvance::SCREEN_HEIGHT);
    input.init();
    gba.reset(filename);	
}

void GBAWindow::pass_input_to_core()
{
    gba.handle_input(input.controller);
    input.controller.input_events.clear();
}

void GBAWindow::core_quit()
{
    gba.mem.save_cart_ram();
    exit(0);   
}

void GBAWindow::run_frame()
{
    gba.run();
    render(gba.disp.screen.data());
}

void GBAWindow::debug_halt()
{
    gba.debug.debug_input();
}

void GBAWindow::core_throttle()
{
    gba.apu.playback.start();
    gba.throttle_emu = true;
}

void GBAWindow::core_unbound()
{
    gba.apu.playback.stop();
    gba.throttle_emu = false; 
}

void GBAWindow::handle_debug()
{
    if(gba.debug.is_halted())
    {
        gba.debug.debug_input();
    }
}