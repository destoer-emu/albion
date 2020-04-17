#ifdef FRONTEND_IMGUI
#include "imgui_window.h"


void Texture::update_texture()
{
    std::scoped_lock<std::mutex> guard(buf_mutex);
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
    std::scoped_lock<std::mutex> guard(buf_mutex);
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



// init imgui
ImguiMainWindow::ImguiMainWindow()
{
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        exit(1);

    // Decide GL+GLSL versions
#if __APPLE__
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    window = glfwCreateWindow(800, 600, "destoer-emu", NULL, NULL);
    if (window == NULL)
        exit(1);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    bool err = gladLoadGL() == 0;
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
    ImGuiIO& io = ImGui::GetIO(); UNUSED(io);
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);  
}


ImguiMainWindow::~ImguiMainWindow()
{
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();    
}

// assume its allways running a gb instance for now
void ImguiMainWindow::start_instance(bool step)
{
    switch(running_type)
    {
        case emu_type::gameboy:
        {
            gameboy_start_instance(step);
            break;
        }

        case emu_type::gba:
        {
            gba_start_instance(step);
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

void ImguiMainWindow::debug_halt()
{
    switch(running_type)
    {
        case emu_type::gameboy:
        {       
            gb.debug.step_instr = true;
            break;
        }

        case emu_type::gba:
        {
            gba.debug.step_instr = true;
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

void ImguiMainWindow::file_browser()
{
    static std::string file_path = std::filesystem::current_path().string(); 
    static int selected = -1;
    static std::string selected_file = "";
    static std::vector<std::string> dir_list = read_sorted_directory(file_path);
    static char input_path[128] = "";
    static bool use_bios = false;


    ImGui::Begin("file browser");


    ImGui::Text("Current dir: %s",file_path.c_str());

    if(ImGui::Button("load rom"))
    {
        if(selected != -1)
        {
            if(std::filesystem::is_regular_file(selected_file))
            {
                new_instance(selected_file,use_bios);
                selected_window = current_window::screen;
            }
        }
    }

    ImGui::SameLine();


    if(ImGui::Button("load state"))
    {
        if(selected != -1)
        {
            if(std::filesystem::is_regular_file(selected_file))
            {
                stop_instance();

                try
                {  
                    load_state(selected_file);
                }

                

                catch(std::exception &ex)
                {
                    std::cout << ex.what() << "\n";
                }
                start_instance();                
            }
        }
    }

    ImGui::SameLine();


    if(ImGui::Button("save state") && *input_path != '\0')
    {
        stop_instance();
        std::string loc = file_path + "/" + std::string(input_path);
        try
        {
           save_state(loc);
        }

        catch(std::exception &ex)
        {
            std::cout << ex.what() << "\n";
        }
        dir_list = read_sorted_directory(file_path);
        *input_path = '\0'; 
        start_instance();
    }

    ImGui::SameLine();

	if (ImGui::Button("../"))
	{  

        selected = -1;
        selected_file = "";

        std::filesystem::path p(file_path);
        file_path = p.parent_path().string();
        dir_list = read_sorted_directory(file_path);
	}


    ImGui::SameLine();

    ImGui::Checkbox("use bios",&use_bios);


	
	if (ImGui::Button("change dir"))
	{  
        if(std::filesystem::is_directory(input_path))
        {
            selected = -1;
            selected_file = "";
            file_path = input_path;
            *input_path = '\0';
            dir_list = read_sorted_directory(file_path);
        }
	}

    ImGui::SameLine();

    ImGui::InputText("", input_path, IM_ARRAYSIZE(input_path));



    ImGui::BeginChild("file view");


    
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
                if(std::filesystem::is_directory(dir_list[i]))
                {
                    selected = -1;
                    selected_file = "";
                    file_path = dir_list[i];
                    dir_list = read_sorted_directory(file_path);
					break;
                }

                else if(std::filesystem::is_regular_file(dir_list[i]))
                {
                    new_instance(dir_list[i],use_bios);
                    selected_window = current_window::screen;
                }
            }   
        }
    }

    ImGui::EndChild();

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
            }

            else
            {
                gb_display_viewer.enabled = false;
            }

            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("File"))
        {
            selected_window = current_window::file;
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

void ImguiMainWindow::mainloop()
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


    
    // and fix gekkio test failures call_iming ret_timing <-- timing issue with vblank and hblank

    /*TODO*/
    // finish sound impl test each channel against a few games and get it up to par with old version
    // impl channel 4 fully!
    // metroid 2 gbc has now broken i think its becuase we added sound!?
    


    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if(running_type == emu_type::gameboy) 
        {
            menu_bar(gb.debug);

            switch(selected_window)
            {
                case current_window::file:
                {
                    file_browser();
                    break;
                }

                case current_window::cpu:
                {
                    gameboy_draw_cpu_info();
                    break;
                }


                case current_window::breakpoint:
                {
                    gameboy_draw_breakpoints();
                    break;
                }


                case current_window::memory:
                {
                    gameboy_draw_memory();
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
        }

        else if(running_type == emu_type::gba)
        { 
            menu_bar(gba.debug);

            switch(selected_window)
            {
                case current_window::file:
                {
                    file_browser();
                    break;
                }

                case current_window::cpu:
                {
                    gba_draw_cpu_info();
                    break;
                }


                case current_window::breakpoint:
                {
                    gba_draw_breakpoints();
                    break;
                }


                case current_window::memory:
                {
                    gba_draw_memory();
                    break;
                }

                case current_window::display_viewer:
                {
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
        }

        else if(running_type == emu_type::none)
        {
            file_browser();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        // setup the viewport so it will stop drawing just below the menubar
        glViewport(0, 0, display_w, display_h-static_cast<GLsizei>(menubar_size.y));

        screen.update_texture();
        screen.draw_texture();

        // and set it back to how imgui expects it
        glViewport(0, 0, display_w, display_h);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
    stop_instance();
}
#endif