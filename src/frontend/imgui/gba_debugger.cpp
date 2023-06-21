#include "imgui_window.h"
#include <albion/destoer-emu.h>

using namespace gameboyadvance;




void ImguiMainWindow::gba_run_frame()
{
    try
    {
        //auto start = std::chrono::system_clock::now();
        gba.handle_input(input.controller);

        gba.run();

        if(gba.disp.new_vblank)
        {
            // swap the buffer so the frontend can render it
            screen.swap_buffer(gba.disp.screen);
        }
    #ifdef DEBUG
        if(gba_display_viewer.enabled)
        {
            gba_display_viewer.update(gba);
        }
    #endif
        //auto end = std::chrono::system_clock::now();
        //printf("fps: %d\n",1000 / std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() );
    }
    

    catch(std::exception &ex)
    {
        gba.debug.write_logger(ex.what());
        std::cout << ex.what() << "\n";
        emu_running = false;
        SDL_GL_SetSwapInterval(1); // Enable vsync
        return;
    }    
}


void ImguiMainWindow::gba_stop_instance()
{
    gba.quit = true;
    emu_running = false;
    gba.mem.save_cart_ram();
}

void ImguiMainWindow::gba_start_instance()
{
    emu_running = true;
    gba.debug.wake_up();
}

void ImguiMainWindow::gba_new_instance(std::string filename)
{
    try
    {
        gba_reset_instance(filename);
        gba_start_instance();
    }

    catch(std::exception &ex)
    {
        std::cout << ex.what()  << "\n";
        return;
    }          
}


void ImguiMainWindow::gba_reset_instance(std::string filename)
{
    try
    {
        gba.reset(filename);
    }

    catch(std::exception &ex)
    {
        std::cout << ex.what()  << "\n";
        return;
    }    
}


#ifdef DEBUG

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


void ImguiMainWindow::gba_draw_cpu_info()
{
    ImGui::Begin("Gba cpu");

    ImGui::Text("current instr: %s",gba.cpu.is_thumb? "thumb" : "arm"); ImGui::SameLine();
    ImGui::Text(",current mode: %s",mode_names[static_cast<int>(gba.cpu.arm_mode)]);

    ImGui::BeginChild("gba cpu left pane", ImVec2(150, 0), true);
    gba_draw_registers_child();
    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChild("gba cpu right pane");
    gba.debug.draw_console();
    ImGui::EndChild();

    ImGui::End();   
}

// add switching between different register windows :P
void ImguiMainWindow::gba_draw_registers_child()
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

/*
void ImguiMainWindow::gba_draw_screen()
{
	ImGui::Begin("gba-screen");
	screen.update_texture();
	ImGui::Image((void*)(intptr_t)screen.get_texture(), ImVec2(gameboyadvance::SCREEN_WIDTH * 2, gameboyadvance::SCREEN_HEIGHT * 2));
	ImGui::End();
}
*/

#endif