#pragma once

#ifdef FRONTEND_IMGUI


// dear imgui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <stdio.h>
#include <destoer-emu/lib.h>
#include <destoer-emu/emulator.h>



// About OpenGL function loaders: modern OpenGL doesn't have a standard header file and requires individual function pointers to be loaded manually.
// Helper libraries are often used for this purpose! Here we are supporting a few common ones: gl3w, glew, glad.
// You may use another loader/header of your choice (glext, glLoadGen, etc.), or chose to manually implement your own.
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h>    // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h>    // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h>  // Initialize with gladLoadGL()
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif

// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h>

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

inline void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}


#include <gb/gb.h>
#include <gba/gba.h>

class Texture
{
public:
    void init_texture(const int X, const int Y);
    void update_texture();
    void swap_buffer(std::vector<uint32_t> &other);
    GLuint get_texture() const;
    void draw_texture();
    std::mutex &get_mutex();
    int get_width() const { return x; } 
    int get_height() const { return y; } 
private:
    std::mutex buf_mutex;
    int x;
    int y;
    std::vector<uint32_t> buf;
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

    std::atomic_bool enabled = false;
    Texture bg_map;
    Texture tiles;

    std::atomic_bool bg_map_higher = false;

private:
    std::mutex pal_mutex;
    uint32_t palette_bg[32];
    uint32_t palette_sp[32];
};


class GBADisplayViewer
{
public:
    GBADisplayViewer();
    void update(gameboyadvance::GBA &gb);
    void draw_palette();


    std::atomic_bool enabled = false;

private:
    static constexpr int PAL_X = 16;
    static constexpr int PAL_Y = 16;
    std::array<uint32_t,PAL_X*PAL_Y> palette{0};

    std::mutex pal_mutex;
};

class ImguiMainWindow
{
public:
    ImguiMainWindow();
    ~ImguiMainWindow();
    void mainloop(); 
private:

    void start_instance(bool step = false);
    void stop_instance();
    void disable_audio();
    void enable_audio();
    void debug_halt();
    void reset_instance(std::string filename, bool use_bios = false);
    void new_instance(std::string filename, bool use_bios = false);
    void load_state(std::string filename);
    void save_state(std::string filename);


    void file_browser();
    void menu_bar(Debug &debug);

    // Gameboy

    // emulator managment
    void gameboy_stop_instance();
    void gameboy_start_instance(bool step = false);
    void gameboy_new_instance(std::string filename, bool use_bios);
    void gameboy_reset_instance(std::string filename, bool use_bios);
    void gameboy_emu_instance();

    // frontend drawing
    void gameboy_draw_screen(); // unused now we just render to back of window

    // display viewer handled by seperate class

    void gameboy_draw_regs_child();
    void gameboy_draw_disassembly_child();
    void gameboy_draw_breakpoints();
    void gameboy_draw_memory();
    void gameboy_draw_cpu_info();

    // gba
    void gba_stop_instance();
    void gba_start_instance(bool step = false);
    void gba_new_instance(std::string filename);
    void gba_reset_instance(std::string filename);
    void gba_emu_instance();

    // frotend drawing
    //void gba_draw_screen();
    void gba_draw_disassembly_child();
    void gba_draw_registers_child(); 
    void gba_draw_cpu_info();
    void gba_draw_breakpoints();
    void gba_draw_memory();


    ImVec2 menubar_size;

     
    
    // underlying emulator instance data
    gameboy::GB gb;
    gameboyadvance::GBA gba;


    enum class current_window
    {
        screen,
        display_viewer,
        cpu,
        memory,
        file,
        breakpoint,
        full_debugger
    };


    current_window selected_window = current_window::file;
    
    GLFWwindow* window;
    std::thread emu_thread;
    std::atomic_bool emu_running = false;  
    emu_type running_type = emu_type::none;

    Texture screen;
    GameboyDisplayViewer gb_display_viewer;
    GBADisplayViewer gba_display_viewer;
};

#endif