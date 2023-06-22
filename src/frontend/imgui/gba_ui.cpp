#include "imgui_window.h"
#include <gba/gba.h>
#include <albion/destoer-emu.h>
using namespace gameboyadvance;
#include "gba_ui.h"


void GBAWindow::start_instance()
{
    emu_running = true;
    gba.debug.wake_up();    
}

void GBAWindow::reset_instance(const std::string& name, b32 use_bios)
{
    UNUSED(use_bios);

    gba_display_viewer.init();
    screen.init_texture(gameboyadvance::SCREEN_WIDTH,gameboyadvance::SCREEN_HEIGHT);
    gba.reset(name);
}

void GBAWindow::stop_instance()
{
    gba.quit = true;
    gba.mem.save_cart_ram();
    emu_running = false;    
}

void GBAWindow::enable_audio()
{
    gba.apu.playback.start();
}

void GBAWindow::disable_audio()
{
    gba.apu.playback.stop();
}

void GBAWindow::throttle_core()
{
    gba.apu.playback.start();
    gba.throttle_emu = true;
}

void GBAWindow::unbound_core()
{
    gba.apu.playback.stop();
    gba.throttle_emu = false;     
}

void GBAWindow::run_frame()
{
    gba.debug.log_enabled = log_enabled;

    try
    {
        gba.handle_input(input.controller);
  
        gba.run();
    #ifdef DEBUG
        if(display_viewer_enabled)
        {
            gba_display_viewer.update(gba);
        }
    #endif
        if(gba.disp.new_vblank)
        {
            // swap the buffer so the frontend can render it
            screen.swap_buffer(gba.disp.screen);
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

void GBAWindow::cpu_info_ui()
{
    ImGui::Begin("Gba cpu");

    ImGui::Text("current instr: %s",gba.cpu.is_thumb? "thumb" : "arm"); ImGui::SameLine();
    ImGui::Text(",current mode: %s",mode_names[static_cast<int>(gba.cpu.arm_mode)]);

    ImGui::BeginChild("gba cpu left pane", ImVec2(150, 0), true);
    draw_regs_child();
    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChild("gba cpu right pane");
    gba.debug.draw_console();
    ImGui::EndChild();

    ImGui::End();   
}

void GBAWindow::draw_regs_child()
{
    enum class reg_window_type
    {
        user_regs,
        current_regs,
        status_regs,
        high_regs,
        fiq_regs,
    };

    static reg_window_type window_type = reg_window_type::user_regs;

    auto constexpr SIZE = 5;
    static const char * window_names[SIZE] = 
    {
        "user_regs",
        "current_regs",
        "status_regs",
        "high_regs",
        "fiq_regs",
    };

    static int selected = 0;

    // combo box to select view type
    if(ImGui::BeginCombo("gba_reg_combo",window_names[selected]))
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
        case reg_window_type::user_regs:
        {
            for(int i = 0; i < 16; i++)
            {
                ImGui::Text("%-3s: %08x ",user_regs_names[i],gba.cpu.user_regs[i]);    
            }
            break;
        }

        case reg_window_type::current_regs:
        {
            for(int i = 0; i < 16; i++)
            {
                ImGui::Text("%-3s: %08x ",user_regs_names[i],gba.cpu.regs[i]);    
            }
            break;
        }

        case reg_window_type::status_regs:
        {
            for(int i = 0; i < 5; i++)
            {
                ImGui::Text("%s: %08x ",status_banked_names[i],gba.cpu.status_banked[i]);    
            }
            break;
        }

        case reg_window_type::high_regs:
        {
            for(int i = 0; i < 5; i++)
            {
                ImGui::Text("%s: %08x ", hi_banked_names[i][0], gba.cpu.hi_banked[i][0]);
                ImGui::Text("%s: %08x ", hi_banked_names[i][1], gba.cpu.hi_banked[i][1]);
			}
			break;
		}

        case reg_window_type::fiq_regs:
		{
			for (int i = 0; i < 5; i++)
			{
				ImGui::Text("%-7s: %08x ", fiq_banked_names[i], gba.cpu.fiq_banked[i]);
			}
			break;
		}
	}

	ImGui::Text("cpsr: %08x ", gba.cpu.get_cpsr());   
} 

void GBAWindow::breakpoint_ui()
{
    breakpoint_ui_internal(gba.debug);
}


const uint64_t GBA_MEM_REGION_SIZE = 9;

MemRegion GBA_MEM_REGION_TABLE[GBA_MEM_REGION_SIZE] =
{
    {"bios 00000000-00003fff",0,0x4000},
    {"onboard wram 02000000-0203ffff",0x02000000,0x40000},
    {"onchip wram 03000000-03007fff",0x03000000,0x8000},
    {"io 04000000-040003ff",0x43000000,0x400},
    {"obj ram 05000000-050003ff",0x05000000,0x400},
    {"vram 06000000-060017fff",0x06000000,0x18000},
    {"oam 07000000-070003ff",0x07000000,0x400},
    {"rom 08000000-09ffffff",0x08000000,32*1024*1024},
    {"sram 0e000000-0e00ffff",0x0e000000,0x10000},
};



void GBAWindow::memory_viewer()
{
    draw_memory_internal(gba.debug,GBA_MEM_REGION_TABLE,GBA_MEM_REGION_SIZE);
}


void GBADisplayViewer::init()
{
    for(auto &t: bg_maps)
    {
        t.init_texture(512,512);
    }
}

void GBADisplayViewer::update(GBA &gba)
{
    gba.disp.render_palette(palette.data(),palette.size());    
    for(int i = 0; i < 4; i++)
    {
        gba.disp.render_map(i,bg_maps[i].buf);
    }
}


void GBADisplayViewer::draw_map()
{
    ImGui::Begin("gba bg map"); 

    const int SIZE = 4;
    static const char * window_names[SIZE] = 
    {
        "bg 0",
        "bg 1",
        "bg 2",
        "bg 3"
    };

    static int selected = 0;

    // combo box to select view type
    if(ImGui::BeginCombo("gba_map_combo",window_names[selected]))
    {
        for(int i = 0; i < SIZE; i++)
        {
            if(ImGui::Selectable(window_names[i],selected == i))
            {
                selected = i;
            }
        }
        ImGui::EndCombo();
    }

    auto &t = bg_maps[selected];
    t.update_texture();
    ImGui::Image((void*)(intptr_t)t.get_texture(),ImVec2(t.get_width(),t.get_height()));    
    ImGui::End();    
}

void GBADisplayViewer::draw_palette()
{
    ImGui::Begin("gba palette");
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    auto p = ImGui::GetCursorScreenPos();
	for(int row = 0; row < PAL_Y; row++)
	{
		for(int col = 0; col < PAL_X; col++)
		{
            float sz = 20.0;
            float x = (p.x + (col * sz));
            float y = (p.y + (row * sz));
            draw_list->AddRectFilled(ImVec2(x+4.0, y+4.0), ImVec2(x+sz, y + sz),  palette[(row*PAL_X)+col]);
		}
	}
    ImGui::End();        
}



void GBAWindow::display_viewer_ui()
{
    draw_screen();
    gba_display_viewer.draw_map();
    gba_display_viewer.draw_palette();
}




void GBAWindow::draw_screen()
{
    ImGui::Begin("gba screen");   
    screen.update_texture();        
    ImGui::Image((void*)(intptr_t)screen.get_texture(),ImVec2(screen.get_width(),screen.get_height()));    
    ImGui::End();    
}