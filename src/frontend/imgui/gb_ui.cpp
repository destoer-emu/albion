#include "imgui_window.h"
#include <gb/gb.h>
#include <albion/destoer-emu.h>
using namespace gameboy;
#include "gb_ui.h"


void GBWindow::start_instance()
{
    emu_running = true;
    gb.debug.wake_up();    
}

void GBWindow::reset_instance(const std::string& name, b32 use_bios)
{
    gb_display_viewer.init();
    screen.init_texture(gameboy::SCREEN_WIDTH,gameboy::SCREEN_HEIGHT);
    gb.reset(name,true,use_bios);
}

void GBWindow::stop_instance()
{
    gb.quit = true;
    gb.mem.save_cart_ram();
    emu_running = false;    
}

void GBWindow::load_state(const std::string& filename)
{
    gb.load_state(filename);
}



void GBWindow::save_state(const std::string& filename)
{
    gb.save_state(filename);
}

void GBWindow::enable_audio()
{
    gb.apu.playback.start();
}

void GBWindow::disable_audio()
{
    gb.apu.playback.stop();
}

void GBWindow::run_frame()
{
    gb.debug.log_enabled = log_enabled;

    try
    {
        gb.handle_input(input.controller);
  
        gb.run();
    #ifdef DEBUG
        if(display_viewer_enabled)
        {
            gb_display_viewer.update(gb);
        }
    #endif
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

void GBWindow::throttle_core()
{
    gb.apu.playback.start();
    gb.throttle_emu = true;
}

void GBWindow::unbound_core()
{
    gb.apu.playback.stop();
    gb.throttle_emu = false;     
}

void GBWindow::cpu_info_ui()
{
    ImGui::Begin("Cpu");
    ImGui::BeginChild("left pane", ImVec2(150, 0), true);
    draw_regs_child();
    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChild("right pane");
    gb.debug.draw_console();
    ImGui::EndChild();
    
    ImGui::End();
}

void GBWindow::draw_regs_child()
{
    // print regs
    ImGui::Text("pc: %04x",gb.cpu.pc);
    ImGui::Text("af: %04x",gb.cpu.read_af());
    ImGui::Text("bc: %04x",gb.cpu.bc);
    ImGui::Text("de: %04x",gb.cpu.de);
    ImGui::Text("hl: %04x",gb.cpu.hl);
    ImGui::Text("sp: %04x",gb.cpu.sp);          
} 

void GBWindow::breakpoint_ui()
{
    breakpoint_ui_internal(gb.debug);
}

const uint64_t GAMEBOY_MEM_REGION_SIZE = 7;

MemRegion GAMEBOY_MEM_REGION_TABLE[GAMEBOY_MEM_REGION_SIZE] =
{
    {"rom 0000-7fff",0,0x8000},
    {"vram 8000-9fff",0x8000,0x2000},
    {"sram a000-bfff",0xa000,0x2000},
    {"wram c000-dfff",0xc000,0x2000},
    {"echo ram e000-fdff",0xe000,0x1e00},
    {"oam fe00-fe9f",0xfe00,0xa0},
    {"io ff00-ffff",0xff00,0x100},
};


void GBWindow::memory_viewer()
{
    draw_memory_internal(gb.debug,GAMEBOY_MEM_REGION_TABLE,GAMEBOY_MEM_REGION_SIZE);
}


void GBWindow::display_viewer_ui()
{
    draw_screen();
    gb_display_viewer.draw_tiles();
    gb_display_viewer.draw_bg_map();
    gb_display_viewer.draw_palette();
}

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
    ImGui::Begin("palette");
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    auto p = ImGui::GetCursorScreenPos();

    for(int cgb_pal = 0; cgb_pal < 8; cgb_pal++)
    {
        for(int color_num = 0; color_num < 4; color_num++)
        {

            float sz = 20.0;
            float x = (p.x + (color_num * sz));
            float y = (p.y + (cgb_pal * sz));

            draw_list->AddRectFilled(ImVec2(x+4.0, y+4.0), ImVec2(x+sz, y + sz),  palette_bg[(cgb_pal*4)+color_num]);
            draw_list->AddRectFilled(ImVec2(x+4.0+(sz*5), y+4.0), ImVec2(x+(sz*6), y + sz),  palette_sp[(cgb_pal*4)+color_num]);
        }
    }
    
    ImGui::End();        
}

void GBWindow::draw_screen()
{
    ImGui::Begin("gameboy screen");   
    screen.update_texture();        
    ImGui::Image((void*)(intptr_t)screen.get_texture(),ImVec2(screen.get_width(),screen.get_height()));    
    ImGui::End();    
}