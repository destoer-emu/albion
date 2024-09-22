#pragma once
#include <gba/gba.h>
#include "sdl_window.h"

class GBAWindow final : public SDLMainWindow
{
protected:
    void init(const std::string& filename,Playback& playback) override;
    void pass_input_to_core() override;
    void run_frame() override;
    void handle_debug() override;
    void core_quit() override;
    void core_throttle() override;
    void core_unbound() override;
    void debug_halt() override;

private:
    gameboyadvance::GBA gba;
};