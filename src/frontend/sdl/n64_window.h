#pragma once
#include <n64/n64.h>
#include "sdl_window.h"

class N64Window final : public SDLMainWindow
{
protected:
    void init(const std::string& filename) override;
    void pass_input_to_core() override;
    void run_frame() override;
    void handle_debug() override;
    void core_quit() override;
    void core_throttle() override;
    void core_unbound() override;
    void debug_halt() override;

private:
    nintendo64::N64 n64;
};
