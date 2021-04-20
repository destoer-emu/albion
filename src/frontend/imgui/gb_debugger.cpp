#include "imgui_window.h"
#include <gb/gb.h>
#include <destoer-emu/destoer-emu.h>
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


// we will switch them in and out but for now its faster to just copy it
void ImguiMainWindow::gameboy_run_frame()
{
    try
    {
        gb_controller.update(gb);
  
        gb.run();

        if(gb_display_viewer.enabled)
        {
            gb_display_viewer.update(gb);
        }

        if(gb.ppu.new_vblank)
        {
            // swap the buffer so the frontend can render it
            screen.swap_buffer(gb.ppu.screen);
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


void ImguiMainWindow::gameboy_stop_instance()
{
    gb.quit = true;
    gb.mem.save_cart_ram();
    emu_running = false;
}

void ImguiMainWindow::gameboy_start_instance()
{
    emu_running = true;
    gb.debug.wake_up();
}

void ImguiMainWindow::gameboy_new_instance(std::string filename, bool use_bios)
{
    try
    {
        gameboy_reset_instance(filename,use_bios);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return; 
    }
    
   
    gameboy_start_instance();     
}


void ImguiMainWindow::gameboy_reset_instance(std::string filename,bool use_bios)
{

    gb.reset(filename,true,use_bios);
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
    
    gb.debug.draw_console();
    ImGui::End();
}

void ImguiMainWindow::gameboy_draw_regs_child()
{
    // print regs
    ImGui::Text("pc: %04x",gb.cpu.read_pc());
    ImGui::Text("af: %04x",gb.cpu.read_af());
    ImGui::Text("bc: %04x",gb.cpu.read_bc());
    ImGui::Text("de: %04x",gb.cpu.read_de());
    ImGui::Text("hl: %04x",gb.cpu.read_hl());
    ImGui::Text("sp: %04x",gb.cpu.read_sp());          
}

// TODO: factor this off
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
        gb.cpu.exec_instr_no_debug();
        gb.debug.halt();
    }


    ImGui::SameLine();

    if(ImGui::Button("Continue"))
    {
        // perform a step with breakpoints off so we can
        // bypass the current instr
        const auto old = gb.debug.breakpoints_enabled;
        gb.debug.breakpoints_enabled = false;
        gb.cpu.exec_instr();
        gb.debug.breakpoints_enabled  = old;

        gb.debug.wake_up();
    }


    ImGui::SameLine();

    if(ImGui::Button("Goto pc"))
    {
        addr = gb.cpu.read_pc();
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
            bool is_pc = target == gb.cpu.read_pc();
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

