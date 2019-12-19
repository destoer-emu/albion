#include "imgui_window.h"

#ifdef FRONTEND_IMGUI

// init imgui
ImguiMainWindow::ImguiMainWindow(std::string filename)
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
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version); 

    mainloop(filename);   
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
void bind_texture(GLuint &texture, std::vector<uint32_t> &buf, int x, int y)
{
    glEnable(GL_TEXTURE_2D); 

    glGenTextures(1,&texture);

    glBindTexture(GL_TEXTURE_2D,texture);

    // setup our texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // create the texture supposedly theres a faster way to do this with subimage but i cant get
    // it working?
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,x,y,0,GL_RGBA,GL_UNSIGNED_BYTE,buf.data());

    glBindTexture(GL_TEXTURE_2D,0);

    glDisable(GL_TEXTURE_2D); 
}


void emu_instance(GB &gb, std::string filename)
{
	constexpr uint32_t fps = 60; 
	constexpr uint32_t screen_ticks_per_frame = 1000 / fps;
	uint64_t next_time = current_time() + screen_ticks_per_frame;
    gb.reset(filename);
    //init_sdl();



try
{
    while(!gb.quit)
    {
        //handle_input();

        gb.run();

        //render();

		// throttle the emulation
        std::this_thread::sleep_for(std::chrono::milliseconds(time_left(next_time)));
		next_time += screen_ticks_per_frame;
    }
}

catch(std::exception &ex)
{
    std::cout << ex.what();
}
}


void ImguiMainWindow::destroy_instance()
{
    if(emu_running)
    {
        gb.quit = true;
        emu_thread.join(); // end the thread
        emu_running = false;
        gb.quit = false;
    }
}

void ImguiMainWindow::mainloop(std::string filename)
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
    GLuint screen_texture;

    std::thread emulator(emu_instance,std::ref(gb),filename);
    emu_running = true;
    std::swap(emulator,emu_thread);

    std::vector<uint32_t> test;
    test.resize(160*144);
    std::fill(test.begin(),test.end(),0xffffffff);

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

        ImGui::Begin("screen"); // <--- figure out why this doesent draw then add syncing and only showing debug info during a pause
        bind_texture(screen_texture,gb.ppu.screen,gb.ppu.X,gb.ppu.Y);
        ImGui::Image((void*)(intptr_t)screen_texture,ImVec2(gb.ppu.X*2,gb.ppu.Y*2));
        ImGui::End();


        // print regs
        ImGui::Begin("registers");
        ImGui::Text("pc: %04x",gb.cpu.get_pc());
        ImGui::Text("sp: %04x",gb.cpu.get_sp());
        ImGui::Text("af: %04x",gb.cpu.get_af());
        ImGui::Text("de: %04x",gb.cpu.get_de());
        ImGui::Text("bc: %04x",gb.cpu.get_bc());
        ImGui::Text("hl: %04x",gb.cpu.get_hl());
        ImGui::End();        



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
    destroy_instance();
}
#endif