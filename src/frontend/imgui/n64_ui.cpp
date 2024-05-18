#ifdef N64_ENABLED
#include "n64_ui.h"
using namespace nintendo64;

void N64Window::start_instance()
{
    emu_running = true;
    n64.debug.wake_up();    
}

void N64Window::reset_instance(const std::string& name, b32 use_bios)
{
    UNUSED(use_bios);
    screen.init_texture(640,480);
    reset(n64,name);
}

void N64Window::stop_instance()
{
    n64.quit = true;
    emu_running = false;    
}


void N64Window::run_frame()
{
    n64.debug.log_enabled = log_enabled;

    try
    {
        nintendo64::handle_input(n64,input.controller);
  
        run(n64);

        if(n64.size_change)
        {
            screen.init_texture(n64.rdp.screen_x,n64.rdp.screen_y);
            n64.size_change = false;
        }

        if(n64.rdp.frame_done)
        {
            // swap the buffer so the frontend can render it
            screen.swap_buffer(n64.rdp.screen);
        }
    }

    catch(std::exception &ex)
    {
        std::cout << ex.what() << "\n";
        emu_running = false;
        SDL_GL_SetSwapInterval(1); // Enable vsync
        return;
    }    
}


void N64Window::cpu_info_ui()
{
    ImGui::Begin("N64 cpu");

    ImGui::BeginChild("n64 cpu left pane", ImVec2(150, 0), true);
    draw_regs_child();
    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChild("n64 cpu right pane");
    n64.debug.draw_console();
    ImGui::EndChild();

    ImGui::End();   
}

void N64Window::draw_regs_child()
{
    enum class reg_window_type
    {
        gpr,
        fpr,
    };

    static reg_window_type window_type = reg_window_type::gpr;

    auto constexpr SIZE = 2;
    static const char * window_names[SIZE] = 
    {
        "gpr",
        "fpr",
    };

    static int selected = 0;

    // combo box to select view type
    if(ImGui::BeginCombo("n64_reg_combo",window_names[selected]))
    {
        for(int i = 0; i < SIZE; i++)
        {
            if(ImGui::Selectable(window_names[i],selected == i))
            {
                selected = i;
                window_type = static_cast<reg_window_type>(selected);
            }
        }
        ImGui::EndCombo();
    }

    switch(window_type)
    {
        case reg_window_type::gpr:
        {
            for(int i = 0; i < 32; i++)
            {
                ImGui::Text("%-3s: %016lx ",nintendo64::reg_name(i),n64.cpu.regs[i]);    
            }
            break;
        }

        case reg_window_type::fpr:
        {
            for(int i = 0; i < 32; i++)
            {
                ImGui::Text("f%02d: %f ",i,n64.cpu.cop1.regs[i]);    
            }
            break;
        }

	}
} 

void N64Window::breakpoint_ui()
{
    breakpoint_ui_internal(n64.debug);
}

const uint64_t N64_MEM_REGION_SIZE = 4;

// NOTE: these have the virtual addresses
MemRegion N64_MEM_REGION_TABLE[N64_MEM_REGION_SIZE] =
{
    {"RDRAM 00000000-03ef'ffff",0xa000'0000,0x03ef'ffff},
    {"RDRAM REGS 0x03F0'0000-0x03FF'FFFF",0xa3F0'0000,0xF'FFFF},
    {"RCP REGS 0x0400'0000-0x04FF'FFFF",0xa400'0000,0xFF'FFFF},
    {"SI BUS 0x1FC0'0000-1FCF'FFFF",0xaFC0'0000,0xF'FFFF},
};



void N64Window::memory_viewer()
{
    //draw_memory_internal(n64.debug,N64_MEM_REGION_TABLE,N64_MEM_REGION_SIZE);
}


#endif