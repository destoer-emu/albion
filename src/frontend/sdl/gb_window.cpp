#include <gb/gb.h>
#include "gb_window.h"


void GameboyWindow::init(const std::string& filename)
{
    init_sdl(gameboy::SCREEN_WIDTH,gameboy::SCREEN_HEIGHT);
    input.init();
    gb.reset(filename);	
}

void GameboyWindow::pass_input_to_core()
{
    gb.handle_input(input.controller);
    input.controller.input_events.clear();
}

void GameboyWindow::core_quit()
{
    gb.mem.save_cart_ram();
    exit(0);   
}

void GameboyWindow::run_frame()
{
    gb.run();
    render(gb.ppu.screen.data());
}

void GameboyWindow::debug_halt()
{
    gb.debug.debug_input();
}

void GameboyWindow::core_throttle()
{
    gb.apu.playback.start();
    gb.throttle_emu = true;
}

void GameboyWindow::core_unbound()
{
    gb.apu.playback.stop();
    gb.throttle_emu = false; 
}

void GameboyWindow::handle_debug()
{
    if(gb.debug.is_halted())
    {
        gb.debug.debug_input();
    }
}