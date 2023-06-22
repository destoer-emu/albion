#ifdef FRONTEND_IMGUI
#include "imgui_window.h"

#ifdef GB_ENABLED
#include "gb_ui.h"
#endif 

#ifdef GBA_ENABLED
#include "gba_ui.h"
#endif

#ifdef N64_ENABLED
#include "n64_ui.h"
#endif

#include <albion/destoer-emu.h>

void ImguiMainWindow::handle_input()
{
    const b32 ignore_key_inputs = ImGui::GetIO().WantCaptureKeyboard;

    const auto control = input.handle_input(context.window,ignore_key_inputs);

    switch(control)
    {
        case emu_control::quit_t:
        {
            quit = true;
            break;
        }

        case emu_control::throttle_t:
        {
            SDL_GL_SetSwapInterval(1);
            throttle_core();
            break;
        }

        case emu_control::unbound_t:
        {
            SDL_GL_SetSwapInterval(0);
            unbound_core();
            break;
        }

        case emu_control::break_t: break;

        case emu_control::none_t: break;
    }
}

void ImguiMainWindow::breakpoint_ui_internal(Debug& debug)
{

	static char input_breakpoint[12] = "";
    static bool break_r = false;
    static bool break_x = false;
    static bool break_w = false;
    static bool enabled[3] = {false};
    static int selected = -1;
    static uint64_t addr_selected;
	ImGui::Begin("Breakpoints");


	ImGui::Checkbox("read", &break_r);
	ImGui::SameLine(); ImGui::Checkbox("write", &break_w);
	ImGui::SameLine(); ImGui::Checkbox("execute", &break_x);


    
    const int type_idx = static_cast<int>(running_type);

    const bool old = enabled[type_idx];

    ImGui::SameLine(); ImGui::Checkbox("enable_all",&enabled[type_idx]);

    if(old != enabled[type_idx])
    {
        debug.change_breakpoint_enable(enabled[type_idx]);
    }
    


	ImGui::InputText("breakpoint-input", input_breakpoint, IM_ARRAYSIZE(input_breakpoint),ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);


	// set breakpoints
	ImGui::SameLine();

	if (ImGui::Button("Set"))
	{
        errno = 0;
        uint64_t breakpoint = strtoll(input_breakpoint, NULL, 16);

        if(errno == 0)
        {
            debug.set_breakpoint(breakpoint,break_r,break_w,break_x,false,false,false);
            *input_breakpoint = '\0';
        }	
	}


    auto &breakpoints = debug.breakpoints;

    if(ImGui::Button("disable"))
    {
        if(selected != -1)
        {
            breakpoints[addr_selected].disable();
        }
    }

    ImGui::SameLine();

    if(ImGui::Button("enable"))
    {
        if(selected != -1)
        {
            breakpoints[addr_selected].enable();
        }
    }

    ImGui::SameLine();

    if(ImGui::Button("delete"))
    {
        if(selected != -1)
        {
            breakpoints.erase(addr_selected);
            selected = -1; // gone from list so deselect it
        }
    }

	ImGui::Separator();

    ImGui::BeginChild("breakpoint list");

    // print breakpoints here
    int i = 0;
    for(auto &it : breakpoints)
    {
        i++;
        Breakpoint b = it.second;    
    
        std::string break_str = fmt::format(
            "{:04x}: {}{}{} {} {:x} {}",b.addr,
                b.break_setting & static_cast<int>(break_type::read)? "r" : "",
                b.break_setting & static_cast<int>(break_type::write)? "w" : "",
                b.break_setting & static_cast<int>(break_type::execute)? "x" : "",
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

void ImguiMainWindow::render_ui()
{
    menu_bar();
    handle_file_ui();

    switch(selected_window)
    {
        case current_window::cpu: cpu_info_ui(); break;
        case current_window::breakpoint: breakpoint_ui(); break;
        case current_window::memory: memory_viewer(); break;
        case current_window::display_viewer: display_viewer_ui(); break;
        default: break;
    }
}

void ImguiMainWindow::render_screen()
{
    // calc offseting and display the screen
    int display_w, display_h;
    SDL_GetWindowSize(context.window, &display_w, &display_h);

    // lets figure out how much to scale by
    const int y = display_h;
    const int x = display_w;

    int fact_y = y / screen.get_height();
    int fact_x = x / screen.get_width();

    // make sure the factor we scale by is equal
    if(fact_x > fact_y)
    {
        fact_x = fact_y;
    }

    else if(fact_y > fact_x)
    {
        fact_y = fact_x;
    }

    // recalc the heights with the new factor
    int screen_y = screen.get_height() * fact_y;
    int screen_x = screen.get_width() * fact_x;



    // now get half the remainder of our total draw area 
    // and use it to keep it in the centre of the viewpoert
    int width_offset = (x - screen_x) / 2;
    int height_offset = (y - screen_y) / 2;

    screen.update_texture();
    screen.draw_texture(width_offset,height_offset,fact_x,fact_y);
}

void ImguiMainWindow::new_instance(const std::string& filename,b32 use_bios)
{
    const auto old = running_type;

    try
    {
        running_type = get_emulator_type(filename);
    }

    catch(std::exception &ex)
    {
        std::cout << ex.what() << "\n";
        return;
    }

    // TODO: allow switching types
    if(running_type != old)
    {
        quit = true;
        exit_filename = filename;
        load_new_rom = true;
        return;
    }

    // start another of the same instance
    try
    {
        reset_instance(filename,use_bios);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return; 
    }
    
    start_instance();   
}

void ImguiMainWindow::mainloop(const std::string &rom_name)
{
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);


    FpsCounter fps;

    if(rom_name != "")
    {
        new_instance(rom_name,false);
    }

    while(!quit)
    {
        fps.reading_start();

        const bool focus = SDL_GetWindowFlags(context.window) & SDL_WINDOW_MOUSE_FOCUS;

        handle_input();

        if(emu_running)
        {
            run_frame();
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        if(focus)
        {
            render_ui();
        }

        // calc offseting and display the screen
        int display_w, display_h;
        SDL_GetWindowSize(context.window, &display_w, &display_h);


        // render the screen texture
        render_screen();

        // render the imgui frame
        ImGui::Render();
        

        // Rendering
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);


        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(context.window);

        fps.reading_end();

        SDL_SetWindowTitle(context.window,fmt::format("albion: {}",fps.get_fps()).c_str());
    }

    stop_instance();
}

// TODO: allow starting another instance up
void mainloop(std::string filename)
{
    ImguiContext context;

    b32 quit = false;

    while(!quit)
    {

        try
        {
            const auto type = get_emulator_type(filename);
        

            switch(type)
            {
            #ifdef GB_ENABLED
                case emu_type::gameboy:
                {
                    GBWindow window(context,emu_type::gameboy);
                    window.mainloop(filename);
                    filename = window.exit_filename;
                    quit = !window.load_new_rom;
                    break;
                }
            #endif

            #ifdef GBA_ENABLED
                case emu_type::gba:
                {
                    GBAWindow window(context,emu_type::gba);
                    window.mainloop(filename);
                    filename = window.exit_filename;
                    quit = !window.load_new_rom;
                    break;
                }
            #endif

            #ifdef N64_ENABLED
                case emu_type::n64:
                {
                    N64Window window(context,emu_type::n64);
                    window.mainloop(filename);
                    filename = window.exit_filename;
                    quit = !window.load_new_rom;
                    break;
                }
            #endif

                default:
                {
                    std::cout << "unrecognised rom type" << filename << "\n";

                    ImguiMainWindow window(context,emu_type::none);
                    window.mainloop(filename);
                    filename = window.exit_filename;
                    break;
                }
            }
        }

        catch(std::exception &ex)
        {
            std::cout << ex.what() << "\n";
            return;
        }
    }    
}


#endif