#include "imgui_window.h"
#include <gb/gb.h>
using namespace gameboy;


// looks like we need to use imgui for the key input :P
void gameboy_handle_input(GB &gb)
{
    // buggy causes movement stutters in metroid but thats probably to be expected
    // taking input on a thread like this from imgui...
    const uint8_t joypad_state = gb.cpu.joypad_state;

    static constexpr int scancodes[] = {GLFW_KEY_A,GLFW_KEY_S,GLFW_KEY_ENTER,GLFW_KEY_SPACE,
        GLFW_KEY_RIGHT,GLFW_KEY_LEFT,GLFW_KEY_UP,GLFW_KEY_DOWN};
    
    static constexpr int gb_key[] = {4,5,7,6,0,1,2,3};

    static constexpr int len = sizeof(scancodes) / sizeof(scancodes[0]);

    static_assert(sizeof(scancodes) == sizeof(gb_key));

    for(int i = 0; i < len; i++)
    {
        if(ImGui::IsKeyDown(scancodes[i]))
        {
            if(is_set(joypad_state,gb_key[i]))
            {
                gb.key_pressed(gb_key[i]);
            }
        }

        // aint pressed
        else if(!is_set(joypad_state,gb_key[i]))
        {
            gb.key_released(gb_key[i]);  
        }
    }

    if(ImGui::IsKeyDown(GLFW_KEY_KP_ADD))
    {
        gb.apu.stop_audio();
        gb.throttle_emu = false;
    }

    else if(ImGui::IsKeyDown(GLFW_KEY_KP_SUBTRACT))
    {
        gb.apu.start_audio();
        gb.throttle_emu = true;						
    }
}

// we will switch them in and out but for now its faster to just copy it
void gameboy_emu_instance(GB &gb, Texture &screen)
{
	constexpr uint32_t fps = 60; 
	constexpr uint32_t screen_ticks_per_frame = 1000 / fps;
	uint64_t next_time = current_time() + screen_ticks_per_frame;

    try
    {
        while(!gb.quit)
        {
            gameboy_handle_input(gb);
            
            
            gb.run();

            // swap the buffer so the frontend can render it
            screen.swap_buffer(gb.ppu.screen);

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
        // halt the emu instance so it can be terminated properly
        gb.debug.halt(); 
    }
}


void ImguiMainWindow::gameboy_stop_instance()
{
    if(emu_running)
    {
        gb.quit = true;
        gb.debug.disable_everything();
        emu_thread.join(); // end the thread
        emu_running = false;
        gb.quit = false;
        gb.mem.save_cart_ram();
    }
}

void ImguiMainWindow::gameboy_start_instance()
{
    std::thread emulator(gameboy_emu_instance,std::ref(gb), std::ref(screen));
    emu_running = true;
    std::swap(emulator,emu_thread);    
    gb.quit = false;
    gb.debug.breakpoints_enabled = true;
}

void ImguiMainWindow::gameboy_new_instance(std::string filename)
{
    gameboy_reset_instance(filename);
    gameboy_start_instance();     
}


void ImguiMainWindow::gameboy_reset_instance(std::string filename)
{
    try
    {
        gb.reset(filename);
    }

    catch(std::exception &ex)
    {
        std::cout << ex.what()  << "\n";
        return;
    }    
}


void ImguiMainWindow::gameboy_draw_screen()
{
    ImGui::Begin("screen"); // <--- figure out why this doesent draw then add syncing and only showing debug info during a pause    
    screen.update_texture();        
    ImGui::Image((void*)(intptr_t)screen.get_texture(),ImVec2(gameboy::SCREEN_WIDTH*2,gameboy::SCREEN_HEIGHT*2));    
    ImGui::End();
}

void ImguiMainWindow::gameboy_draw_regs()
{
    // print regs
    ImGui::Begin("registers");
    ImGui::Text("pc: %04x",gb.cpu.get_pc());
    ImGui::Text("af: %04x",gb.cpu.get_af());
    ImGui::Text("bc: %04x",gb.cpu.get_bc());
    ImGui::Text("de: %04x",gb.cpu.get_de());
    ImGui::Text("hl: %04x",gb.cpu.get_hl());
    ImGui::Text("sp: %04x",gb.cpu.get_sp());
    ImGui::End();            
}


void ImguiMainWindow::gameboy_draw_disassembly()
{
    static uint32_t addr = 0x100; // make this resync when a breakpoint is hit :)
    static int selected = -1;
    static uint32_t selected_addr = 0x0;
    static bool update = false;

    ImGui::Begin("disassembly");

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
        gb.debug.wake_up();
        gb.debug.step_instr = true;
    }


    ImGui::SameLine();

    if(ImGui::Button("Continue"))
    {
        gb.debug.wake_up();
        gb.debug.step_instr = false;
    }

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

            disass_str = fmt::format("{:04x}: {}",target,gb.disass.disass_op(target));
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
    ImGui::End();
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
                ImGui::Text("%02x ",gb.mem.raw_read(((j)+i*0x10)&0xffff));
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


    ImGui::Checkbox("enable_all",&gb.debug.breakpoints_enabled);

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