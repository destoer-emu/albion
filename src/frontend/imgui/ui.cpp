#include "imgui_window.h"

void ImguiMainWindow::menu_bar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if(ImGui::BeginMenu("Emulator"))
        {
            if (ImGui::MenuItem("Pause"))
            {
                stop_instance();
            }

            if (ImGui::MenuItem("Continue")) 
            {
                start_instance();
            }

            if(ImGui::MenuItem("Enable audio"))
            {
                enable_audio();
            }

            if(ImGui::MenuItem("Disable audio"))
            {
                disable_audio();
            }

            if(log_enabled)
            {
                if(ImGui::MenuItem("Disable logger")) 
                {
                    log_enabled = false;
                }
            }

            else
            {
                if(ImGui::MenuItem("Enable logger")) 
                {
                    log_enabled = true;
                }
            }

            ImGui::EndMenu();
        }
    #ifdef DEBUG
        if(ImGui::BeginMenu("Debug"))
        {
            if (ImGui::MenuItem("Cpu"))
            {
                selected_window = current_window::cpu;
            }

            if (ImGui::MenuItem("Memory")) 
            {
                selected_window = current_window::memory;
            }

            if(ImGui::MenuItem("Breakpoints")) 
            {
                selected_window = current_window::breakpoint;
            }
            
            if(ImGui::MenuItem("Display Viewer"))
            {
                selected_window = current_window::display_viewer;
                display_viewer_enabled = true;
            }

            else
            {
                display_viewer_enabled = false;
            }

            ImGui::EndMenu();
        }
    #endif
        if(ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Load rom"))
            {
                selected_window = current_window::load_rom;
            }

            if (ImGui::MenuItem("Load state")) 
            {
                selected_window = current_window::load_state;
            }

            if(ImGui::MenuItem("Save state")) 
            {
                selected_window = current_window::save_state;
            }
            

            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Screen"))
        {
            selected_window = current_window::screen;
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void ImguiMainWindow::handle_file_ui()
{
    // handle common cases
    switch(selected_window)
    {
        case current_window::load_rom: file_browser(file_option::load_rom,"load rom"); break;
        case current_window::load_state: file_browser(file_option::load_state,"load state"); break;
        case current_window::save_state: file_browser(file_option::save_state,"save state"); break;
        default: break;
    }      
}


void ImguiMainWindow::do_file_option(file_option option, const std::string &filename, bool use_bios)
{
    switch(option)
    {
        case file_option::load_rom:
        {
        
            if(!std::filesystem::is_regular_file(filename))
            {
                return;
            }   

            printf("load rom: %s\n",filename.c_str());
            new_instance(filename,use_bios);
            selected_window = current_window::screen;
        
            break;
        
        }

        case file_option::load_state:
        {
        
            if(!std::filesystem::is_regular_file(filename))
            {
                return;
            }   
            stop_instance();
            
            try
            {  
                load_state(filename);
                selected_window = current_window::screen;
            }

            catch(std::exception &ex)
            {
                std::cout << ex.what() << "\n";
                stop_instance();
            }
            start_instance();
        
            assert(false);      
            break;
        }

        case file_option::save_state:
        {
        
            stop_instance();
            try
            {
                printf("save state: %s\n",filename.c_str());
                save_state(filename);
                selected_window = current_window::screen;
            }

            catch(std::exception &ex)
            {
                std::cout << ex.what() << "\n";
            }
            start_instance();
        
            assert(false);
            break;
        }
    }
    
}

void ImguiMainWindow::file_browser(file_option option, const char *title)
{
    static std::string file_path = std::filesystem::current_path().string();
    static char input_path[128] = {'\0'};
    if(!*input_path)
    {
        strncpy(input_path,file_path.c_str(),sizeof(input_path)-1);
    }
    static int selected = -1;
    static std::string selected_file = "";
    static std::vector<std::string> dir_list = read_sorted_directory(file_path);

    // TODO: make this a setting in a config file
    const bool use_bios = false;


    ImGui::Begin(title);

    if(ImGui::InputText("##file_browser_input_text", input_path, IM_ARRAYSIZE(input_path),ImGuiInputTextFlags_EnterReturnsTrue))
    {
        if(std::filesystem::is_directory(input_path))
        {
            selected = -1;
            selected_file = "";
            file_path = input_path;
            dir_list = read_sorted_directory(file_path);
        }        
    }


    ImGui::BeginChild("file view",ImVec2(0, 300), true);

    for(size_t i = 0; i < dir_list.size(); i++)
    {
        // display only the file name
        std::string disp_path = std::filesystem::path(dir_list[i]).filename().string();

        if(ImGui::Selectable(disp_path.c_str(),
            static_cast<size_t>(selected) == i,ImGuiSelectableFlags_AllowDoubleClick))
        {
            selected = i;
            selected_file = dir_list[i];
            if (ImGui::IsMouseDoubleClicked(0))
            {
                // traverse dir
                if(std::filesystem::is_directory(selected_file))
                {
                    file_path = selected_file;
                    dir_list = read_sorted_directory(file_path);
                    strncpy(input_path,file_path.c_str(),sizeof(input_path)-1);
                    break;
                }

                else 
                {
                    do_file_option(option,selected_file,use_bios);
                    break;
                }
                selected = -1;
                selected_file = "";
            }   
        }
    }

    ImGui::EndChild();

    static char input_file[128];

    const bool enter = ImGui::InputText("  ", input_file, IM_ARRAYSIZE(input_file),ImGuiInputTextFlags_EnterReturnsTrue);

    ImGui::SameLine();

    if(ImGui::Button("Go") || enter)
    {
        const auto full_filename = file_path + path_separator + input_file;
        do_file_option(option,full_filename,use_bios);
        *input_file = '\0';
    }

    ImGui::End();
}

void ImguiMainWindow::draw_memory_internal(Debug& debug, MemRegion* region_ptr, u32 size)
{
    if(running_type == emu_type::none)
    {
        return;
    }

    ImGui::Begin("memory-editor");


    static uint64_t region_idx = 0;

    if(region_idx > size)
    {
        size = 0;
    }



    const uint64_t base_addr = region_ptr[region_idx].offset;
    const uint64_t clipper_count = region_ptr[region_idx].size / 0x10;

    static int y = -1;
    static int x = -1;

    // combo box to select view type
    if(ImGui::BeginCombo("##gba-mem-combo",region_ptr[region_idx].name))
    {
        for(uint64_t i = 0; i < size; i++)
        {
            if(ImGui::Selectable(region_ptr[i].name,region_idx == i))
            {
                region_idx = i;
                // we have just changed reset to the top of the memory viewer
                ImGui::SetScrollY(0.0);
                x = -1;
                y = -1;
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    ImGui::Text("edit: %zx",base_addr + (y * 0x10) + x); ImGui::SameLine();
    static char input[3] = {0};
    ImGui::PushItemWidth(20.0);
    if(ImGui::InputText("##mem-write", input, IM_ARRAYSIZE(input),ImGuiInputTextFlags_EnterReturnsTrue | 
        ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase))
    {
        if(x != -1 && y != -1)
        {
            debug.write_mem(base_addr + (y * 0x10) + x,strtol(input,NULL,16));
            *input = '\0';
            x += 1;
            if(x == 0x10)
            {
                y++;
                x = 0;
            }
            // keep in text box after input
            ImGui::SetKeyboardFocusHere(-1);
        }
    }
    ImGui::PopItemWidth();

    // draw col
    ImGui::Text("          "); ImGui::SameLine();

    ImGui::BeginTable("offsets",0x10, ImGuiTableFlags_SizingFixedFit);
    for(int i = 0; i < 0x10; i++)
    {
        ImGui::TableNextColumn();
        ImGui::Text("%02x ",i);
    }
    ImGui::EndTable();
    ImGui::Separator();

    ImGui::BeginChild("Memory View");
    ImGuiListClipper clipper;
    clipper.Begin(clipper_count); 


    while (clipper.Step())
    {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
        {
            ImGui::BeginTable("offsets",0x11, ImGuiTableFlags_SizingFixedFit);
            ImGui::TableNextColumn();
            ImGui::Text("%08zx: ",(base_addr+i*0x10)); 
        

            for(int j = 0; j < 0x10; j++)
            {
                ImGui::TableNextColumn();
                uint64_t dest = (base_addr+j+(i*0x10));
                if(ImGui::Selectable(fmt::format("{:02x} ",debug.read_mem(dest)).c_str(),i == y && j == x,ImGuiSelectableFlags_AllowDoubleClick))
                {
                    y = i;
                    x = j;
                }
            }
            ImGui::EndTable();
        }
    }


    ImGui::EndChild();
    ImGui::End();
}