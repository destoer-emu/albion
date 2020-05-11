#include "imgui_window.h"
#include <gb/gb.h>
#include <frontend/gb/controller.h>
using namespace gameboy;


void GameboyDisplayViewer::init()
{
    bg_map.init_texture(256,256);
    tiles.init_texture(0x10*8*2,0x18*8);
    memset(palette_bg,0,sizeof(palette_bg));
    memset(palette_sp,0,sizeof(palette_sp));
}


void GameboyDisplayViewer::update(GB &gb)
{
    auto t = gb.ppu.render_tiles();
    auto b = gb.ppu.render_bg(bg_map_higher);
    tiles.swap_buffer(t);
    bg_map.swap_buffer(b);
    std::scoped_lock<std::mutex> guard(pal_mutex);
    gb.ppu.render_palette(palette_bg,palette_sp);    
}

void GameboyDisplayViewer::draw_bg_map()
{
    ImGui::Begin("gameboy bg map"); 
    if(ImGui::Button(bg_map_higher? "9C00" : "9800"))
    {
        bg_map_higher = !bg_map_higher;
    }
    bg_map.update_texture();
    ImGui::Image((void*)(intptr_t)bg_map.get_texture(),ImVec2(bg_map.get_width(),bg_map.get_height()));    
    ImGui::End();    
}

void GameboyDisplayViewer::draw_tiles()
{
    ImGui::Begin("gameboy tiles");
    tiles.update_texture();
    ImGui::Image((void*)(intptr_t)tiles.get_texture(),ImVec2(tiles.get_width(),tiles.get_height()));    
    ImGui::End();    
}

void GameboyDisplayViewer::draw_palette()
{
    std::scoped_lock<std::mutex> guard(pal_mutex);
    ImGui::Begin("cgb palette");
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    auto p = ImGui::GetCursorScreenPos();
	for(int cgb_pal = 0; cgb_pal < 8; cgb_pal++)
	{
		for(int color_num = 0; color_num < 4; color_num++)
		{
            // smash a colored box here somehow...
            // think we need to conver it to float first
            // i cant find the api call to draw the box
            float sz = 20.0;
            float x = (p.x + (color_num * sz));
            float y = (p.y + (cgb_pal * sz));
            //uint32_t col = palette_bg[(cgb_pal*4)+color_num];
            draw_list->AddRectFilled(ImVec2(x+4.0, y+4.0), ImVec2(x+sz, y + sz),  palette_bg[(cgb_pal*4)+color_num]);
            draw_list->AddRectFilled(ImVec2(x+4.0+(sz*5), y+4.0), ImVec2(x+(sz*6), y + sz),  palette_sp[(cgb_pal*4)+color_num]);
		}
	}
    ImGui::End();        
}


// looks like we need to use imgui for the key input :P
void gameboy_handle_input(GB &gb)
{
    static constexpr int scancodes[] = {GLFW_KEY_A,GLFW_KEY_S,GLFW_KEY_ENTER,GLFW_KEY_SPACE,
        GLFW_KEY_RIGHT,GLFW_KEY_LEFT,GLFW_KEY_UP,GLFW_KEY_DOWN};
    
    static constexpr button gb_key[] = {button::a,button::b,button::start,
        button::select,button::right,button::left,button::up,button::down};

    static constexpr int len = sizeof(scancodes) / sizeof(scancodes[0]);

    static_assert(sizeof(scancodes) == sizeof(gb_key));

    // cache last state so we can filter by state change
    static bool pressed[len] = {false};

    for(int i = 0; i < len; i++)
    {
        bool down = ImGui::IsKeyDown(scancodes[i]);

        if(down && !pressed[i]) // just pressed
        {
            gb.key_pressed(gb_key[i]);
            pressed[i] = true;
        }

        // just released
        else if(!down &&  pressed[i])
        {
            gb.key_released(gb_key[i]); 
            pressed[i] = false; 
        }
    }

    if(ImGui::IsKeyDown(GLFW_KEY_KP_ADD))
    {
        gb.apu.playback.stop();
        gb.throttle_emu = false;
    }

    else if(ImGui::IsKeyDown(GLFW_KEY_KP_SUBTRACT))
    {
        gb.apu.playback.start();
        gb.throttle_emu = true;						
    }
}

// we will switch them in and out but for now its faster to just copy it
void ImguiMainWindow::gameboy_emu_instance()
{
	constexpr uint32_t fps = 60; 
	constexpr uint32_t screen_ticks_per_frame = 1000 / fps;
	uint64_t next_time = current_time() + screen_ticks_per_frame;

    emu_running = true;
    gb.quit = false;

    try
    {
        GbControllerInput controller;
	    controller.init();
        while(!gb.quit)
        {
            controller.update(gb);

            gameboy_handle_input(gb);
            
            gb.run();

            // swap the buffer so the frontend can render it
            screen.swap_buffer(gb.ppu.screen);
            
            if(gb_display_viewer.enabled)
            {
                gb_display_viewer.update(gb);
            }


            // throttle the emulation
            if(gb.throttle_emu)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(time_left(next_time)));
            }

            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(time_left(next_time) / 8));
            }

            next_time = current_time() + screen_ticks_per_frame;
        }
    }

    catch(std::exception &ex)
    {
        std::cout << ex.what() << "\n";
        emu_running = false;
        return;
    }
    emu_running = false;
}


void ImguiMainWindow::gameboy_stop_instance()
{
    if(emu_thread.joinable())
    {
        gb.quit = true;
        // save the breakpoint state so we can restore it later
        bool break_enabled = gb.debug.breakpoints_enabled;
        gb.debug.disable_everything();
        emu_thread.join(); // end the thread
        gb.quit = false;
        gb.mem.save_cart_ram();
        gb.debug.breakpoints_enabled = break_enabled;
    }
}

void ImguiMainWindow::gameboy_start_instance(bool step)
{
    gb.debug.step_instr = step;
    if(!emu_thread.joinable())
    {
        emu_thread = std::thread(&ImguiMainWindow::gameboy_emu_instance,this);    
    }

    else
    {
        gb.debug.wake_up();
    }
}

void ImguiMainWindow::gameboy_new_instance(std::string filename, bool use_bios)
{
    gameboy_reset_instance(filename,use_bios);
    gameboy_start_instance();     
}


void ImguiMainWindow::gameboy_reset_instance(std::string filename,bool use_bios)
{
    try
    {
        gb.reset(filename,true,use_bios);
    }

    catch(std::exception &ex)
    {
        std::cout << ex.what()  << "\n";
        return;
    }    
}


void ImguiMainWindow::gameboy_draw_screen()
{
    ImGui::Begin("gameboy screen"); // <--- figure out why this doesent draw then add syncing and only showing debug info during a pause    
    screen.update_texture();        
    ImGui::Image((void*)(intptr_t)screen.get_texture(),ImVec2(screen.get_width(),screen.get_height()));    
    ImGui::End();
}


void ImguiMainWindow::gameboy_draw_cpu_info()
{
    ImGui::Begin("Cpu");
    ImGui::BeginChild("left pane", ImVec2(150, 0), true);
    gameboy_draw_regs_child();
    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChild("right pane");
    gameboy_draw_disassembly_child();
    ImGui::EndChild();
    ImGui::End();
}

void ImguiMainWindow::gameboy_draw_regs_child()
{
    // print regs
    ImGui::Text("pc: %04x",gb.cpu.get_pc());
    ImGui::Text("af: %04x",gb.cpu.get_af());
    ImGui::Text("bc: %04x",gb.cpu.get_bc());
    ImGui::Text("de: %04x",gb.cpu.get_de());
    ImGui::Text("hl: %04x",gb.cpu.get_hl());
    ImGui::Text("sp: %04x",gb.cpu.get_sp());          
}


void ImguiMainWindow::gameboy_draw_disassembly_child()
{
    static uint32_t addr = 0x100; // make this resync when a breakpoint is hit :)
    static int selected = -1;
    static uint32_t selected_addr = 0x0;
    static bool update = false;

	static char input_disass[12] = "";
	if (ImGui::Button("Goto"))
	{
		if (is_valid_hex_string(input_disass))
		{
			addr = strtoll(input_disass, NULL, 16) % 0xffff;
            update = true;
			*input_disass = '\0';
		}  
	}


    ImGui::SameLine();

    ImGui::InputText("", input_disass, IM_ARRAYSIZE(input_disass));

    if(ImGui::Button("Break"))
    {
        if(selected != -1)
        {
            gb.debug.set_breakpoint(selected_addr,false,false,true);
        }
    }

    ImGui::SameLine();

    if(ImGui::Button("Step"))
    {
        start_instance(true);
    }


    ImGui::SameLine();

    if(ImGui::Button("Continue"))
    {
        start_instance();
    }


    ImGui::SameLine();

    if(ImGui::Button("Goto pc"))
    {
        addr = gb.cpu.get_pc();
        update = true;
    }

    ImGui::BeginChild("disass view");
    
    // technically could be less than this but we dont know for sure
    constexpr int ITEMS_COUNT = 0x10000; 
    ImGuiListClipper clipper(ITEMS_COUNT); 

    float line_size = ImGui::GetTextLineHeightWithSpacing();

    if(update)
    {
        update = false;
        ImGui::SetScrollY(addr * line_size);
    }


    std::string disass_str;
    while (clipper.Step())
    {
        uint16_t target = clipper.DisplayStart;
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
        {
            bool is_pc = target == gb.cpu.get_pc();
            if(is_pc)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f,0.0f,1.0f,1.0f));
            }

            // for now only show bank over zero target in rom banking range
            // will add vram and wram banks etc later
            int bank = target >= 0x4000  && target <= 0x8000 ? gb.mem.get_bank() : 0;

            disass_str = fmt::format("{:02x}:{:04x}: {}",bank,target,gb.disass.disass_op(target));
            
            if(ImGui::Selectable(disass_str.c_str(),selected == i))
            {
                selected = i;
                selected_addr = target;
            }


            if(is_pc)
            {
                ImGui::PopStyleColor();
            }

            target = (target + gb.disass.get_op_sz(target)) & 0xffff;
        }
    }

    ImGui::EndChild();
}


void ImguiMainWindow::gameboy_draw_memory()
{
    static uint32_t addr = 0x0;
    static uint32_t edit_addr = 0x100;
    static uint32_t edit_value = 0xff;
    static bool update = false;
    ImGui::Begin("Memory editor");


	static char input_mem[12] = "";
	if (ImGui::Button("Goto"))
	{
		if (is_valid_hex_string(input_mem))
		{
			addr = strtoll(input_mem, NULL, 16);
            addr &= 0xfff0; // round to nearest section
            update = true;
			*input_mem = '\0';
		}  
	}

    ImGui::SameLine();

    ImGui::InputText("addr", input_mem, IM_ARRAYSIZE(input_mem));



	static char input_edit[12] = "";
	if (ImGui::Button("edit"))
	{
		if (is_valid_hex_string(input_mem) && is_valid_hex_string(input_edit))
		{
			edit_addr = strtoll(input_mem, NULL, 16) & 0xffff;
            edit_value = strtoll(input_edit,NULL,16) & 0xff;
            *input_edit = '\0';
			*input_mem = '\0';
            gb.mem.raw_write(edit_addr,edit_value);
		}  
	}

    ImGui::SameLine();

    ImGui::InputText("value", input_edit, IM_ARRAYSIZE(input_edit));

    

    // padding
    ImGui::Text("      "); ImGui::SameLine();

    // draw col
    for(int i = 0; i < 0x10; i++)
    {
        ImGui::Text("%02x ",i);
        ImGui::SameLine();
    }

    ImGui::Text("\n");
    ImGui::Separator();


    ImGui::BeginChild("Memory View");
    constexpr int ITEMS_COUNT = 0x10000 / 0x10;
    ImGuiListClipper clipper(ITEMS_COUNT); 

    float line_size = ImGui::GetTextLineHeightWithSpacing();

    if(update)
    {
        update = false;
        ImGui::SetScrollY((addr / 0x10) * line_size);
    }

    while (clipper.Step())
    {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
        {
            
            ImGui::Text("%04x: ",(i*0x10)&0xffff);
            ImGui::SameLine();


            for(int j = 0; j < 0x10; j++)
            {
                ImGui::Text("%02x ",gb.mem.raw_read((j+(i*0x10))&0xffff));
                ImGui::SameLine();
            }
            ImGui::Text("\n");
        }
    }

    ImGui::EndChild();
    ImGui::End();
}

void ImguiMainWindow::gameboy_draw_breakpoints()
{

	static char input_breakpoint[12] = "";
    static bool break_r = false;
    static bool break_x = false;
    static bool break_w = false;
    static int selected = -1;
    static uint32_t addr_selected;
	ImGui::Begin("Breakpoints");


	// look into mutlithreading the emulated instance and the debugger
	// so it can breakpoint pre read / write
	ImGui::Checkbox("read", &break_r);
	ImGui::SameLine(); ImGui::Checkbox("write", &break_w);
	ImGui::SameLine(); ImGui::Checkbox("execute", &break_x);


    ImGui::SameLine(); ImGui::Checkbox("enable_all",&gb.debug.breakpoints_enabled);

	ImGui::InputText("", input_breakpoint, IM_ARRAYSIZE(input_breakpoint));


	// set breakpoints
	ImGui::SameLine();

	if (ImGui::Button("Set"))
	{
		if (is_valid_hex_string(input_breakpoint))
		{
			uint32_t breakpoint = strtoll(input_breakpoint, NULL, 16);

            gb.debug.set_breakpoint(breakpoint,break_r,break_w,break_x);

			*input_breakpoint = '\0';
		}
	}


    if(ImGui::Button("disable"))
    {
        if(selected != -1)
        {
            gb.debug.breakpoints[addr_selected].disable();
        }
    }

    ImGui::SameLine();

    if(ImGui::Button("enable"))
    {
        if(selected != -1)
        {
            gb.debug.breakpoints[addr_selected].enable();
        }
    }

    ImGui::SameLine();

    if(ImGui::Button("delete"))
    {
        if(selected != -1)
        {
            gb.debug.breakpoints.erase(addr_selected);
            selected = -1; // gone from list so deselect it
        }
    }

	ImGui::Separator();

    ImGui::BeginChild("breakpoint list");

    // print breakpoints here
    int i = 0;
    for(auto &it : gb.debug.breakpoints)
    {
        i++;
        Breakpoint b = it.second;    
    
        std::string break_str = fmt::format(
            "{:04x}: {}{}{} {} {:x} {}",b.addr,
                b.r? "r" : "",
                b.w? "w" : "",
                b.x? "x" : "",
                b.break_enabled? "enabled" : "disabled",
                b.value,
                b.value_enabled? "enabled" : "disabled"
        );
        if(ImGui::Selectable(break_str.c_str(),selected == i))
        {
            selected = i;
            // save the addr so we can re index the map later
            addr_selected = b.addr; 
        }
    }

    ImGui::EndChild();

	ImGui::End();	    
}