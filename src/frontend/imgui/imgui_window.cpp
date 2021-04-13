#ifdef FRONTEND_IMGUI
#include "imgui_window.h"
#include <destoer-emu/destoer-emu.h>


void Texture::update_texture()
{
    glEnable(GL_TEXTURE_2D); 
    glBindTexture(GL_TEXTURE_2D,texture);
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,x,y,GL_RGBA, GL_UNSIGNED_BYTE,buf.data());
    glBindTexture(GL_TEXTURE_2D,0);
    glDisable(GL_TEXTURE_2D); 
}


void Texture::init_texture(const int X, const int Y)
{
    x = X;
    y = Y;
    buf.resize(x*y);
    std::fill(buf.begin(),buf.end(),0);

    glEnable(GL_TEXTURE_2D); 
    if(first_time)
    {
        glGenTextures(1,&texture);
        first_time = false;
    }
    glBindTexture(GL_TEXTURE_2D,texture);

    // setup our texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,x,y,0,GL_RGBA, GL_UNSIGNED_BYTE,buf.data());

    glBindTexture(GL_TEXTURE_2D,0);
    glDisable(GL_TEXTURE_2D); 
}

void Texture::swap_buffer(std::vector<uint32_t> &other)
{
    std::swap(other,buf);
}


void Texture::draw_texture()
{
    glEnable(GL_TEXTURE_2D); 
    glBindTexture(GL_TEXTURE_2D,texture);

    // figure out how to maintain ratio here ;)

    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0, 1.0);
        glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0, 1.0);
        glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0, -1.0);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0, -1.0);
    glEnd();

    glBindTexture(GL_TEXTURE_2D,0);
    glDisable(GL_TEXTURE_2D);    
}

GLuint Texture::get_texture() const
{
    return texture;
}

// contructor and destructor code taken from imgui
// https://github.com/ocornut/imgui/blob/master/examples/example_sdl_opengl3/main.cpp

// init imgui
ImguiMainWindow::ImguiMainWindow()
{
    // Setup window
    // Decide GL+GLSL versions
#ifdef __APPLE__
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    window = SDL_CreateWindow("destoer-emu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, window_flags);
    gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    bool err = gladLoadGL() == 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD2)
    bool err = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress) == 0; // glad2 recommend using the windowing library loader instead of the (optionally) bundled one.
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
    bool err = false;
    glbinding::Binding::initialize();
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
    bool err = false;
    glbinding::initialize([](const char* name) { return (glbinding::ProcAddress)SDL_GL_GetProcAddress(name); });
#else
    bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        exit(1);
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);
    
}


ImguiMainWindow::~ImguiMainWindow()
{
    // Cleanup
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

// assume its allways running a gb instance for now
void ImguiMainWindow::start_instance()
{
    switch(running_type)
    {
        case emu_type::gameboy:
        {
            gameboy_start_instance();
            break;
        }

        case emu_type::gba:
        {
            gba_start_instance();
            break;
        }

        case emu_type::none:
        {
            break;
        }
    }
}

void ImguiMainWindow::stop_instance()
{
    switch(running_type)
    {
        case emu_type::gameboy:
        {       
            gameboy_stop_instance();
            break;
        }

        case emu_type::gba:
        {
            gba_stop_instance();
            break;
        }

        case emu_type::none:
        {
            break;
        }        
    }
}

void ImguiMainWindow::reset_instance(std::string filename, bool use_bios)
{
    running_type = get_emulator_type(filename);

    switch(running_type)
    {
        case emu_type::gameboy:
        {
            screen.init_texture(gameboy::SCREEN_WIDTH,gameboy::SCREEN_HEIGHT);    
            gameboy_reset_instance(filename,use_bios);
            break;
        }

        case emu_type::gba:
        {
            screen.init_texture(gameboyadvance::SCREEN_WIDTH,gameboyadvance::SCREEN_HEIGHT);
            gba_reset_instance(filename);
            break;
        }

        case emu_type::none:
        {
            break;
        }

    }
}


void ImguiMainWindow::new_instance(std::string filename, bool use_bios)
{
    stop_instance();


    try
    {
        running_type = get_emulator_type(filename);
    }

    catch(std::exception &ex)
    {
        std::cout << ex.what() << "\n";
        return;
    }

    switch(running_type)
    {
        case emu_type::gameboy:
        {
            screen.init_texture(gameboy::SCREEN_WIDTH,gameboy::SCREEN_HEIGHT);
            gameboy_new_instance(filename,use_bios);
            break;
        }

        case emu_type::gba:
        {
            screen.init_texture(gameboyadvance::SCREEN_WIDTH,gameboyadvance::SCREEN_HEIGHT);
            gba_new_instance(filename);
        }

        case emu_type::none:
        {
            break;
        }
    }
}

void ImguiMainWindow::load_state(std::string filename)
{
    switch(running_type)
    {
        case emu_type::gameboy:
        {
            gb.load_state(filename);
            break;
        }

        case emu_type::gba:
        {
            // ignore unsupported
            break;
        }

        case emu_type::none:
        {
            break;
        }
    }
}

void ImguiMainWindow::save_state(std::string filename)
{
    switch(running_type)
    {
        case emu_type::gameboy:
        {    
            gb.save_state(filename);
            break;
        }

        case emu_type::gba:
        {
            //ignore unsupported
            break;
        }

        case emu_type::none:
        {
            break;
        }

    }
}

std::string pad_string(const std::string &str)
{
    auto copy = str;
    copy.resize(128);
    return copy;
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

    if(ImGui::InputText("", input_path, IM_ARRAYSIZE(input_path),ImGuiInputTextFlags_EnterReturnsTrue))
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



void ImguiMainWindow::enable_audio()
{
    switch(running_type)
    {
        case emu_type::gameboy:
        {
            gb.apu.playback.start();
            break;
        }

        case emu_type::gba:
        {
            gba.apu.playback.start();
            break;
        }

        case emu_type::none:
        {
            break;
        }

    }
}


void ImguiMainWindow::disable_audio()
{
    switch(running_type)
    {
        case emu_type::gameboy:
        {
            gb.apu.playback.stop();
            break;
        }

        case emu_type::gba:
        {
            gba.apu.playback.stop();
            break;
        }

        case emu_type::none:
        {
            break;
        }

    }
}

void ImguiMainWindow::menu_bar(Debug &debug)
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

            if(debug.log_enabled)
            {
                if(ImGui::MenuItem("Disable logger")) 
                {
                    debug.log_enabled = false;
                }
            }

            else
            {
                if(ImGui::MenuItem("Enable logger")) 
                {
                    debug.log_enabled = true;
                }
            }

            ImGui::EndMenu();
        }

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
                gb_display_viewer.enabled = true;
                gba_display_viewer.enabled = true;
            }

            else
            {
                gb_display_viewer.enabled = false;
                gba_display_viewer.enabled = false;
            }

            ImGui::EndMenu();
        }

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

        menubar_size = ImGui::GetWindowSize();

        ImGui::EndMainMenuBar();
    }
}

void ImguiMainWindow::mainloop(const std::string &rom_name)
{


    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'misc/fonts/README.txt' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    screen.init_texture(gameboy::SCREEN_WIDTH,gameboy::SCREEN_HEIGHT);
    gb_display_viewer.init();
    gba_display_viewer.init();
    
    
    FpsCounter fps;

    gb_controller.init();
    gba_controller.init();


    if(rom_name != "")
    {
        new_instance(rom_name,false);
    }

    // Main loop
    bool done = false;
    while (!done)
    {
        fps.reading_start();


        const bool focus = SDL_GetWindowFlags(window) & SDL_WINDOW_MOUSE_FOCUS;

        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            
            switch(event.type)
            {

                case SDL_QUIT:
                {
                    done = true;
                    break;
                }

                case SDL_WINDOWEVENT: 
                {
                    if(event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                    {
                        done = true;
                    }
                    break;
                }

                case SDL_KEYDOWN:
                {
                    switch(running_type)
                    {
                        case emu_type::gameboy:
                        {
                            gb.key_input(event.key.keysym.sym,true);
                            break;
                        }

                        case emu_type::gba:
                        {
                            gba.key_input(event.key.keysym.sym,true);
                            break;
                        }

                        default: break;
                    }

                    break;
                }
                
                case SDL_KEYUP:
                {
                    switch(running_type)
                    {
                        case emu_type::gameboy:
                        {
                            gb.key_input(event.key.keysym.sym,false);
                            break;
                        }

                        case emu_type::gba:
                        {
                            gba.key_input(event.key.keysym.sym,false);
                            break;
                        }

                        default: break;
                    }

                    break;
                }
            }

        }


        
        if(emu_running)
        {
            switch(running_type)
            {
                case emu_type::gameboy: gameboy_run_frame(); break;
                case emu_type::gba: gba_run_frame(); break;
                default: break;
            }
        }

        if(focus)
        {
            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame(window);
            ImGui::NewFrame();

            switch(running_type)
            {
                case emu_type::gameboy:
                {
                    menu_bar(gb.debug);

                    switch(selected_window)
                    {
                        case current_window::load_rom: file_browser(file_option::load_rom,"load rom"); break;
                        case current_window::load_state: file_browser(file_option::load_state,"load state"); break;
                        case current_window::save_state: file_browser(file_option::save_state,"save state"); break;

                        case current_window::cpu:
                        {
                            gameboy_draw_cpu_info();
                            break;
                        }


                        case current_window::breakpoint:
                        {
                            draw_breakpoints();
                            break;
                        }


                        case current_window::memory:
                        {
                            draw_memory();
                            break;
                        }

                        case current_window::display_viewer:
                        {
                            gb_display_viewer.draw_bg_map();
                            gb_display_viewer.draw_tiles();
                            gb_display_viewer.draw_palette();
                            gameboy_draw_screen();
                            break;
                        }

                        case current_window::screen:
                        {
                            break;
                        }

                        case current_window::full_debugger:
                        {
                            break;
                        }
                    }
                    break;
                }

                case emu_type::gba:
                {
                    menu_bar(gba.debug);

                    switch(selected_window)
                    {
                        case current_window::load_rom: file_browser(file_option::load_rom,"load rom"); break;
                        case current_window::load_state: file_browser(file_option::load_state,"load state"); break;
                        case current_window::save_state: file_browser(file_option::save_state,"save state"); break;

                        case current_window::cpu:
                        {
                            gba_draw_cpu_info();
                            break;
                        }


                        case current_window::breakpoint:
                        {
                            draw_breakpoints();
                            break;
                        }


                        case current_window::memory:
                        {
                            draw_memory();
                            break;
                        }

                        case current_window::display_viewer:
                        {
                            gba_display_viewer.draw_palette();
                            gba_display_viewer.draw_map();
                            break;
                        }

                        case current_window::screen:
                        {
                            break;
                        }

                        case current_window::full_debugger:
                        {
                            break;
                        }
                    }
                    break;
                }

                case emu_type::none:
                {
                    file_browser(file_option::load_rom,"load rom");
                    break;
                }
            }
            ImGui::Render();
        }

        

        // Rendering
        int display_w, display_h;
        SDL_GetWindowSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);


        // lets figure out how much to scale by
        const int y = display_h - static_cast<int>(menubar_size.y);
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

        // set the viewport to draw from the offsets and draw the screen for the factor we are strecthing at
        glViewport(width_offset, height_offset, screen_x, screen_y);

        screen.update_texture();
        screen.draw_texture();

        // and set it back to how imgui expects it
        glViewport(0, 0, display_w, display_h);
        
        if(focus)
        {
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        SDL_GL_SwapWindow(window);

        fps.reading_end();

        SDL_SetWindowTitle(window,fmt::format("destoer-emu: {}",fps.get_fps()).c_str());
    }
    stop_instance();
}
#endif