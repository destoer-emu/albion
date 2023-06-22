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
#include <imgui/backends/imgui_impl_sdl2.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <stdio.h>
#include <albion/lib.h>
#include <albion/emulator.h>
#include <albion/debug.h>
#include <frontend/input.h>
#include "texture.h"



class ImguiContext
{
public:
    ImguiContext();
    ~ImguiContext();

    SDL_Window* window;
    SDL_GLContext gl_context;
};


struct MemRegion
{
    MemRegion(const char *n,uint64_t o, uint64_t s) : name(n), offset(o), size(s)
    {}

    const char *name;
    const uint64_t offset;
    const uint64_t size;
};

class ImguiMainWindow
{
public:
    ImguiMainWindow(ImguiContext& c,emu_type type) : running_type(type), context(c)
    {

    }

    void mainloop(const std::string& filename);

    // name for new emu instance?
    std::string exit_filename = "";
    b32 load_new_rom = false;

private:

    enum class file_option
    {
        load_rom,
        load_state,
        save_state
    };


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

    // top level ui
    void handle_input();
    void render_ui();
    void render_screen();

    // files
    void file_browser(file_option option, const char *title);
    void do_file_option(file_option option, const std::string &filename, bool use_bios);
    void handle_file_ui();

    // menu
    void menu_bar();

    

    void new_instance(const std::string& name, b32 use_bios);

protected:

    // emulation control
    virtual void start_instance() {}
    virtual void reset_instance(const std::string& name, b32 use_bios) { UNUSED(name); UNUSED(use_bios); }
    virtual void stop_instance() {}
    virtual void run_frame() {}
    virtual void load_state(const std::string& filename) { UNUSED(filename); }
    virtual void save_state(const std::string& filename) { UNUSED(filename); }
    virtual void enable_audio() {}
    virtual void disable_audio() {}
    virtual void throttle_core() {}
    virtual void unbound_core() {}
    
    // debugging ui
    virtual void cpu_info_ui() {}
    virtual void breakpoint_ui() {}
    virtual void display_viewer_ui() {}
    virtual void memory_viewer() {}

    void breakpoint_ui_internal(Debug& debug);
    void draw_memory_internal(Debug& debug, MemRegion* region_ptr, u32 size);

    // frontend debugging access
    virtual u8 read_mem(u64 addr) { return 0; UNUSED(addr); }
    virtual void write_mem(u64 addr, u8 v) {UNUSED(addr); UNUSED(v);}



    b32 emu_running = false;

    b32 display_viewer_enabled = false;
    b32 log_enabled = false;

    emu_type running_type = emu_type::none;

    ImguiContext& context;
    b32 quit = false;
    Input input;

    Texture screen;
};




void mainloop(std::string rom_name);

#endif