#ifdef FRONTEND_IMGUI
#include "imgui_window.h"



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
    window = glfwCreateWindow(1280, 720, "destoer-emu", NULL, NULL);
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



// can probably optimise this to an init and redraw but screw it for now :P
void update_texture(GLuint &texture, std::vector<uint32_t> &buf, const int x, const int y)
{
    glEnable(GL_TEXTURE_2D); 

    glBindTexture(GL_TEXTURE_2D,texture);

    // setup our texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // create the texture supposedly theres a faster way to do this with subimage but i cant get
    // it working?
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,x,y,0,GL_RGBA, GL_UNSIGNED_BYTE,buf.data());

    glBindTexture(GL_TEXTURE_2D,0);

    glDisable(GL_TEXTURE_2D); 
}

// looks like we need to use imgui for the key input :P
void handle_input(GB &gb)
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
}

// we will switch them in and out but for now its faster to just copy it
void emu_instance(GB &gb, std::vector<uint32_t> &screen_copy, std::mutex &screen_mutex)
{
	constexpr uint32_t fps = 60; 
	constexpr uint32_t screen_ticks_per_frame = 1000 / fps;
	uint64_t next_time = current_time() + screen_ticks_per_frame;

    try
    {
        while(!gb.quit)
        {
            handle_input(gb);
            
            
            gb.run();

            // swap the buffer so the frontend can render it
            {
                std::scoped_lock<std::mutex> guard(screen_mutex);
                std::swap(gb.ppu.screen,screen_copy);
            }

            // throttle the emulation
            std::this_thread::sleep_for(std::chrono::milliseconds(time_left(next_time)));
            next_time += screen_ticks_per_frame;
        }
    }

    catch(std::exception &ex)
    {
        std::cout << ex.what() << "\n";
        // halt the emu instance so it can be terminated properly
        gb.debug.halt(); 
    }
}


void ImguiMainWindow::stop_instance()
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

void ImguiMainWindow::start_instance()
{
    std::thread emulator(emu_instance,std::ref(gb),std::ref(screen_copy),std::ref(screen_mutex));
    emu_running = true;
    std::swap(emulator,emu_thread);    
    gb.quit = false;
    gb.debug.breakpoints_enabled = true;
}


void ImguiMainWindow::draw_screen()
{
    ImGui::Begin("screen"); // <--- figure out why this doesent draw then add syncing and only showing debug info during a pause
    {
        std::scoped_lock<std::mutex> guard(screen_mutex);
        update_texture(screen_texture,screen_copy,gb.ppu.X,gb.ppu.Y);
    }        
    ImGui::Image((void*)(intptr_t)screen_texture,ImVec2(gb.ppu.X*2,gb.ppu.Y*2));    
    ImGui::End();
}

void ImguiMainWindow::draw_regs()
{
    // print regs
    ImGui::Begin("registers");
    ImGui::Text("pc: %04x",gb.cpu.get_pc());
    ImGui::Text("sp: %04x",gb.cpu.get_sp());
    ImGui::Text("af: %04x",gb.cpu.get_af());
    ImGui::Text("de: %04x",gb.cpu.get_de());
    ImGui::Text("bc: %04x",gb.cpu.get_bc());
    ImGui::Text("hl: %04x",gb.cpu.get_hl());
    ImGui::End();            
}

/*
void ImguiMainWindow::draw_menu_bar()
{
    if(ImGui::BeginMainMenuBar())
    {
        if(ImGui::BeginMenu("File"))
        {
            if(ImGui::MenuItem("Load rom"))
            {
                // open menu here
                std::string filename = open_file_dialog(".");

                // nothing back means the dialog was close or an error occured
                if(filename != "") 
                {
                    stop_instance();
                    gb.reset(filename);
                    start_instance();
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}
*/

void ImguiMainWindow::draw_disassembly()
{
    static constexpr int disass_items = 20;
    static uint32_t addr = 0x100; // make this resync when a breakpoint is hit :)
    static int selected = -1;
    static uint32_t selected_addr = 0x0;

    ImGui::Begin("disassembly");

	static char input_disass[12] = "";
	if (ImGui::Button("Goto"))
	{
		if (is_valid_hex_string(input_disass))
		{
			addr = strtoll(input_disass, NULL, 16) % 0xffff;
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

    ImGui::BeginChild("disass view");
    
    uint32_t target = addr;
    std::string disass_str;
    for(int i = 0; i < disass_items; i++)
    {
        target &= 0xffff;
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

        target += gb.disass.get_op_sz(target);
    }

    ImGui::EndChild();
    ImGui::End();
}


// basic logger impl
void ImguiMainWindow::draw_logger()
{
    ImGui::Begin("Logger");

    if(ImGui::Button("clear logs"))
    {
        gb.debug.clear_logs();
    }

    ImGui::BeginChild("logger window");

    for(const auto &x: gb.debug.get_logs())
    {
        ImGui::Text(x.c_str());
    }
    ImGui::EndChild();

    ImGui::End();
}

void ImguiMainWindow::draw_memory()
{
    static uint32_t addr = 0x100;
    static uint32_t edit_addr = 0x100;
    static uint32_t edit_value = 0xff;

    ImGui::Begin("Memory editor");


	static char input_mem[12] = "";
	if (ImGui::Button("Goto"))
	{
		if (is_valid_hex_string(input_mem))
		{
			addr = strtoll(input_mem, NULL, 16);
            addr &= 0xfff0; // round to nearest section
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

    ImGui::BeginChild("Memory view");

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


    for(int i = 0; i < 0x10; i++)
    {
        ImGui::Text("%04x: ",(addr+i*0x10)&0xffff);
        ImGui::SameLine();

        for(int j = 0; j < 0x10; j++)
        {
            ImGui::Text("%02x ",gb.mem.raw_read((addr+j)+i*0x10)&0xffff);
            ImGui::SameLine();        
        }

        ImGui::Text("\n");
    }
    ImGui::EndChild();
    ImGui::End();
}

void ImguiMainWindow::draw_breakpoints()
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

void ImguiMainWindow::file_browser()
{
    static std::string file_path = std::filesystem::current_path(); 
    static int selected = -1;
    static std::string selected_file = "";
    static std::vector<std::string> dir_list = read_sorted_directory(file_path);


    ImGui::Begin("file browser");


    ImGui::Text("Current dir: %s",file_path.c_str());

    if(ImGui::Button("load rom"))
    {
        if(selected != -1)
        {
            if(std::filesystem::is_regular_file(selected_file))
            {
                stop_instance();
                gb.reset(selected_file);
                start_instance();                
            }
        }
    }

    ImGui::SameLine();

	if (ImGui::Button("../"))
	{  

        selected = -1;
        selected_file = "";

        std::filesystem::path p(file_path);
        file_path = p.parent_path();
        dir_list = read_sorted_directory(file_path);
	}

	static char input_dir[50] = "";
	if (ImGui::Button("change dir"))
	{  
        if(std::filesystem::is_directory(input_dir))
        {
            selected = -1;
            selected_file = "";
            file_path = input_dir;
            dir_list = read_sorted_directory(file_path);
        }
	}

    ImGui::SameLine();

    ImGui::InputText("", input_dir, IM_ARRAYSIZE(input_dir));



    ImGui::BeginChild("file view");

    int i = 0;

    for(auto &path: dir_list)
    {
        // display only the file name
        std::string disp_path = std::filesystem::path(path).filename();

        if(ImGui::Selectable(disp_path.c_str(),selected == i,ImGuiSelectableFlags_AllowDoubleClick))
        {
            selected = i;
            selected_file = path;
            if (ImGui::IsMouseDoubleClicked(0))
            {
                if(std::filesystem::is_directory(path))
                {
                    selected = -1;
                    selected_file = "";
                    file_path = path;
                    dir_list = read_sorted_directory(file_path);
                }

                else if(std::filesystem::is_regular_file(path))
                {
                    new_instance(path);
                }
            }
            
        }

        i++;
    }

    ImGui::EndChild();

    ImGui::End();
}

void ImguiMainWindow::new_instance(std::string filename)
{
    stop_instance();
    try
    {
        gb.reset(filename);
    }

    catch(std::exception &ex)
    {
        std::cout << ex.what()  << "\n";
        return;
    }

    start_instance();     
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

    // init our screen buffer
    screen_copy.resize(gb.ppu.X*gb.ppu.Y);
    glGenTextures(1,&screen_texture);

    gb.reset("",false);


    // after fix links awakening crash
    // and fix gekkio test failures call_iming ret_timing
    // also figure out why exceptions crash this version of the code

    // and finally redo sound (dont just copy code for it cause its a very messy impl)
    // use inheritance

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

        if(true) // running in debug mode flag!
        {
            //draw_menu_bar();
            draw_screen();
            draw_regs();
            draw_disassembly();
            draw_breakpoints();
            draw_memory();
            draw_logger();
            file_browser();
        }

        // <---- START HERE!
        // get this working then begin work on breakpoints
        // disassembly and memory viewing
        // make debugger or play mode changable from menu bar
        // add ppu and sound viewers later :)
        else // render jsut the main game fullscreen
        {

        }


        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
    stop_instance();
}
#endif