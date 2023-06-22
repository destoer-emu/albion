#pragma once
#include "imgui_window.h"
#include <n64/n64.h>


class N64Window final : public ImguiMainWindow
{
public:
    using ImguiMainWindow::ImguiMainWindow;

protected:
    // emu control
    virtual void start_instance() override; 
    virtual void reset_instance(const std::string& name, b32 use_bios) override; 
    virtual void stop_instance() override;
    virtual void run_frame() override; 


private:
    nintendo64::N64 n64;
};