#pragma once

#ifdef FRONTEND_IMGUI


// taken from imgui
// https://github.com/ocornut/imgui/blob/master/examples/example_sdl_opengl3/main.cpp

// dear imgui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)



// About Desktop OpenGL function loaders:
//  Modern desktop OpenGL doesn't have a standard portable header file to load OpenGL function pointers.
//  Helper libraries are often used for this purpose! Here we are supporting a few common ones (gl3w, glew, glad).
//  You may use another loader/header of your choice (glext, glLoadGen, etc.), or chose to manually implement your own.
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h>            // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h>            // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h>          // Initialize with gladLoadGL()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD2)
#include <glad/gl.h>            // Initialize with gladLoadGL(...) or gladLoaderLoadGL()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
#define GLFW_INCLUDE_NONE       // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/Binding.h>  // Initialize with glbinding::Binding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
#define GLFW_INCLUDE_NONE       // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/glbinding.h>// Initialize with glbinding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif


#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <stdio.h>
#include <destoer-emu/lib.h>
#include <destoer-emu/emulator.h>
#include <frontend/gb/controller.h>
#include <frontend/gba/controller.h>





#include <gb/gb.h>
#include <gba/gba.h>
#include <n64/n64.h>

class Texture
{
public:
    void init_texture(const int X, const int Y);
    void update_texture();
    void swap_buffer(std::vector<uint32_t> &other);
    GLuint get_texture() const;
    void draw_texture();
    int get_width() const { return x; } 
    int get_height() const { return y; } 

    std::vector<uint32_t> buf;

private:
    int x;
    int y;
    GLuint texture;
    bool first_time = true;
};


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

class ImguiMainWindow
{
public:
    ImguiMainWindow();
    ~ImguiMainWindow();
    void mainloop(const std::string &rom_name); 
private:

    void start_instance();
    void stop_instance();
    void disable_audio();
    void enable_audio();
    void new_instance(std::string filename, bool use_bios = false);
    void load_state(std::string filename);
    void save_state(std::string filename);

    void handle_file_ui();

    enum class file_option
    {
        load_rom,
        load_state,
        save_state
    };

    void do_file_option(file_option option, const std::string &filename, bool use_bios);
    void file_browser(file_option option, const char *title);
    void menu_bar(Debug &debug);
#ifdef DEBUG
    std::unordered_map<uint64_t,Breakpoint> &get_breakpoint_ref();
    void draw_breakpoints();

    void write_mem(uint64_t addr,uint8_t v);
    uint8_t read_mem(uint64_t addr);
    void draw_memory();
#endif
    // Gameboy

    // emulator managment
    void gameboy_stop_instance();
    void gameboy_start_instance();
    void gameboy_new_instance(std::string filename, bool use_bios);
    void gameboy_reset_instance(std::string filename, bool use_bios);
    void gameboy_run_frame();
#ifdef DEBUG
    // frontend drawing
    void gameboy_draw_screen(); // unused now we just render to back of window

    // display viewer handled by seperate class

    void gameboy_draw_regs_child();
    void gameboy_draw_memory();
    void gameboy_draw_cpu_info();
#endif
    // gba
    void gba_stop_instance();
    void gba_start_instance();
    void gba_new_instance(std::string filename);
    void gba_reset_instance(std::string filename);
    void gba_run_frame();
#ifdef DEBUG
    // frotend drawing
    void gba_draw_registers_child(); 
    void gba_draw_cpu_info();
    void gba_draw_memory();
#endif

    void n64_stop_instance();
    void n64_start_instance();
    void n64_new_instance(std::string filename, bool use_bios = false);
    void n64_reset_instance(std::string filename, bool use_bios = false);
    void n64_run_frame();


    ImVec2 menubar_size;

     
    
    // underlying emulator instance data
    gameboy::GB gb;
    gameboyadvance::GBA gba;
    nintendo64::N64 n64;
    GbControllerInput gb_controller;
    GbaControllerInput gba_controller;

    enum class current_window
    {
        screen,
        load_rom,
        load_state,
        save_state,
    #ifdef DEBUG
        display_viewer,
        cpu,
        memory,
        breakpoint,
    #endif
    };


    current_window selected_window = current_window::load_rom;
    
    SDL_Window* window;
    SDL_GLContext gl_context;
    bool emu_running = false; 
    emu_type running_type = emu_type::none;

    Texture screen;
#ifdef DEBUG
    GameboyDisplayViewer gb_display_viewer;
    GBADisplayViewer gba_display_viewer;
#endif
};

#endif