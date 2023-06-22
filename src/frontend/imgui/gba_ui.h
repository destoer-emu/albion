#pragma once
#include "imgui_window.h"
#include <gba/gba.h>


class GBADisplayViewer
{
public:
    void init();
    void update(gameboyadvance::GBA &gb);
    void draw_palette();
    void draw_map();


    bool enabled = false;

private:
    static constexpr int PAL_X = 16;
    static constexpr int PAL_Y = 16;
    std::array<uint32_t,PAL_X*PAL_Y> palette{0};

    std::array<Texture,4> bg_maps;
};


class GBAWindow final : public ImguiMainWindow
{
public:
    using ImguiMainWindow::ImguiMainWindow;

protected:
    // emu control
    virtual void start_instance() override; 
    virtual void reset_instance(const std::string& name, b32 use_bios) override; 
    virtual void stop_instance() override;
    virtual void run_frame() override; 
    virtual void enable_audio() override;
    virtual void disable_audio() override;
    virtual void throttle_core() override;
    virtual void unbound_core() override;

    // debug ui
    void cpu_info_ui() override;
    void breakpoint_ui() override;
    void memory_viewer() override;
    void display_viewer_ui() override;


    void draw_regs_child();


    void draw_screen();

private:
    gameboyadvance::GBA gba;
    GBADisplayViewer gba_display_viewer;
};
