#include <gb/gb.h>
#include "gb_window.h"


void GameboyWindow::init(const std::string& filename,Playback& playback)
{
    init_sdl(gameboy::SCREEN_WIDTH,gameboy::SCREEN_HEIGHT);
    input.init();
    gb.reset(filename);	
    gb.apu.audio_buffer.playback = &playback;
    playback.init(gb.apu.audio_buffer);
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
    render(gb.ppu.rendered.data());
}

void GameboyWindow::debug_halt()
{
    gb.debug.debug_input();
}

void GameboyWindow::core_throttle()
{
    playback.start();
    reset_audio_buffer(gb.apu.audio_buffer);
    gb.throttle_emu = true;
}

void GameboyWindow::core_unbound()
{
    playback.stop();
    gb.throttle_emu = false; 
}

void GameboyWindow::handle_debug()
{
    if(gb.debug.is_halted())
    {
        gb.debug.debug_input();
    }
}