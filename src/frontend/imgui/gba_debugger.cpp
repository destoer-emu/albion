#include "imgui_window.h"
#include <destoer-emu/destoer-emu.h>

using namespace gameboyadvance;


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
    if(ImGui::BeginCombo("",window_names[selected]))
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


void ImguiMainWindow::gba_run_frame()
{
    try
    {
        //auto start = std::chrono::system_clock::now();
        gba_controller.update(gba);

        gba.run();

        if(gba.disp.new_vblank)
        {
            // swap the buffer so the frontend can render it
            screen.swap_buffer(gba.disp.screen);
        }

        if(gba_display_viewer.enabled)
        {
            gba_display_viewer.update(gba);
        }

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


void ImguiMainWindow::gba_draw_cpu_info()
{
    ImGui::Begin("Gba cpu");

    ImGui::Text("current instr: %s",gba.cpu.is_cpu_thumb()? "thumb" : "arm");

    ImGui::BeginChild("gba cpu left pane", ImVec2(150, 0), true);
    gba_draw_registers_child();
    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChild("gba cpu right pane");
    gba_draw_disassembly_child();
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
    if(ImGui::BeginCombo("",window_names[selected]))
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
                ImGui::Text("%-3s: %08x ",user_regs_names[i],gba.cpu.get_user_regs(i));    
            }
            break;
        }

        case reg_window_type::current_regs:
        {
            for(int i = 0; i < 16; i++)
            {
                ImGui::Text("%-3s: %08x ",user_regs_names[i],gba.cpu.get_current_regs(i));    
            }
            break;
        }

        case reg_window_type::status_regs:
        {
            for(int i = 0; i < 5; i++)
            {
                ImGui::Text("%s: %08x ",status_banked_names[i],gba.cpu.get_status_regs(i));    
            }
            break;
        }

        case reg_window_type::high_regs:
        {
            for(int i = 0; i < 5; i++)
            {
                ImGui::Text("%s: %08x ", hi_banked_names[i][0], gba.cpu.get_high_regs(i, 0));
                ImGui::Text("%s: %08x ", hi_banked_names[i][1], gba.cpu.get_high_regs(i, 1));
			}
			break;
		}

        case reg_window_type::fiq_regs:
		{
			for (int i = 0; i < 5; i++)
			{
				ImGui::Text("%-7s: %08x ", fiq_banked_names[i], gba.cpu.get_fiq_regs(i));
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

// TODO we are getting the wrong pc now because of the pipline :P
void ImguiMainWindow::gba_draw_disassembly_child() 
{
	static constexpr uint32_t MAX_ADDR = 0x0E010000;
	static uint32_t base_addr = 0;
	static uint32_t addr = 0x08000000; // make this resync when a breakpoint is hit :)
	static int selected = -1;
	static uint32_t selected_addr = 0x0;
	static bool update = true;
    static bool is_thumb = false;

	static char input_disass[12] = "";
	if (ImGui::Button("Goto"))
	{
		if (is_valid_hex_string(input_disass))
		{
			addr = strtoll(input_disass, NULL, 16) % MAX_ADDR;
			update = true;
			*input_disass = '\0';
		}
	}
	ImGui::SameLine();

	ImGui::InputText("", input_disass, IM_ARRAYSIZE(input_disass));
	if (ImGui::Button("Break"))
	{
		if (selected != -1)
		{
			gba.debug.set_breakpoint(selected_addr, false, false, true);
		}
	}
	ImGui::SameLine();

	if (ImGui::Button("Step"))
	{
        gba.cpu.exec_instr_no_debug();
        gb.debug.halt();
	}
	ImGui::SameLine();

	if (ImGui::Button("Continue"))
	{
        // perform a step with breakpoints off so we can
        // bypass the current instr
        const auto old = gba.debug.breakpoints_enabled;
        gba.debug.breakpoints_enabled = false;
        gba.cpu.exec_instr();
        gba.debug.breakpoints_enabled  = old;

        gba.debug.wake_up();
	}

    ImGui::SameLine();


	if (ImGui::Button("Goto pc"))
	{
		addr = gba.cpu.get_pc() % MAX_ADDR;
		update = true;
	}
    ImGui::SameLine();
    // change view
    if(ImGui::Button(is_thumb? "arm" : "thumb"))
    {
        is_thumb = !is_thumb;
        update = true;
    }

	ImGui::BeginChild("disass view gba");

	// we are gonna cull some of this manually
	// as MAX_ADDR / 2 is too large for it to handle!
	static constexpr uint32_t CLIPPER_COUNT = 1024 * 1024;

    // 2 for thumb instrs 4 for arm instrs
    int bytes_per_line = is_thumb? 2 : 4;

	// offset for base address so that the target ends up in the middle of the scroll region :)
	const uint32_t CLIPPER_ADDR_OFFSET = (CLIPPER_COUNT * bytes_per_line) / 2;
	ImGuiListClipper clipper(CLIPPER_COUNT);
	
	if (update)
	{
		update = false;

		// if addr underflows set to 0
		if (addr - CLIPPER_ADDR_OFFSET > addr)
		{
			base_addr = 0;
		}

		
		else
		{
			base_addr = addr - CLIPPER_ADDR_OFFSET;
		}


		// scroll to the target
		int line = (addr - base_addr) / bytes_per_line;
        const float line_size = ImGui::GetTextLineHeightWithSpacing();
        ImGui::SetScrollY(line * line_size);
    }


    std::string disass_str;
    while (clipper.Step())
    {
        uint32_t target = (clipper.DisplayStart*bytes_per_line) + base_addr;
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
        {
			// color instr at current pc blue
            const bool is_pc = target == gba.cpu.get_pc();
            if(is_pc)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f,0.0f,1.0f,1.0f));
            }

            disass_str = fmt::format("{:08x}: {}",target,
                is_thumb? gba.disass.disass_thumb(target) : gba.disass.disass_arm(target) 
            );
            if(ImGui::Selectable(disass_str.c_str(),selected == i))
            {
                selected = i;
                selected_addr = target;
            }


            if(is_pc)
            {
                ImGui::PopStyleColor();
            }

            target = (target + bytes_per_line) % MAX_ADDR;
        }
    }

    ImGui::EndChild();
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