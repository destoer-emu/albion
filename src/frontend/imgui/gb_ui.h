#pragma once
#include "imgui_window.h"
#include <gb/gb.h>

class GameboyDisplayViewer
{
public:
    void init();
    void update(gameboy::GB &gb);

    void draw_bg_map();
    void draw_tiles();
    void draw_palette();

    bool enabled = false;
    Texture bg_map;
    Texture tiles;

    bool bg_map_higher = false;

private:
    uint32_t palette_bg[32];
    uint32_t palette_sp[32];
};

class GBWindow final : public ImguiMainWindow
{
public:
    using ImguiMainWindow::ImguiMainWindow;

protected:
    // emu control
    virtual void start_instance() override; 
    virtual void reset_instance(const std::string& name, b32 use_bios) override; 
    virtual void stop_instance() override;
    virtual void run_frame() override; 
    virtual void load_state(const std::string& filename) override;
    virtual void save_state(const std::string& filename) override;
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
    gameboy::GB gb;
    GameboyDisplayViewer gb_display_viewer;
};