#include <destoer-emu/destoer-emu.h>
#include <gb/gb.h>
// #include <gba/gba.h> GBA is not good enough to port yet it needs fire emblem fixed, and a HLE bios and probably a touch more optimisation

// okay lets get a basic android UI up and going
// Backed by SDL and rendered by hand

enum class current_window_t
{
    load_rom,
};

// base texture low res 16:9 we are going to let SDL just scale it
// atm this is large enough for any of the systems we have
static constexpr u32 X = 640;
static constexpr u32 Y = 360;

struct AndroidUI
{
    AndroidUI();
    ~AndroidUI();

    void render_frame();

    void draw_text(const std::string &str, u32 x, u32 y);

    current_window_t current_window;

	SDL_Window * window = NULL;
	SDL_Renderer * renderer = NULL;
	SDL_Texture * texture = NULL;

    bool is_horizontal;
    u32 x;
    u32 y;

    std::vector<u32> screen;    
};


AndroidUI::AndroidUI()
{

    // these should be pulled via SDL for now we are going to just fudge them so we can test it on the deskop
    const u32 width = X * 2;
    const u32 height = Y * 2;

    is_horizontal = width > height;

/*
    SDL_DisplayMode display_mode;

    SDL_GetCurrentDisplayMode(0,&display_mode);

    const u32 width = display_mode.w;
    const u32 height = display_mode.h;
*/

	// initialize our window
	window = SDL_CreateWindow("destoer-emu",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,width,height,SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
	
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl"); // crashes without this on windows?
	
	// set a render for our window
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	texture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_BGR888, SDL_TEXTUREACCESS_STREAMING, X, Y);

    screen.resize(X * Y);

    std::fill(screen.begin(),screen.end(),0xfffffff);        
}

AndroidUI::~AndroidUI()
{
	if(renderer)
	{
    	SDL_DestroyRenderer(renderer);
	}

	if(window)
	{
    	SDL_DestroyWindow(window);
	}

	if(texture)
	{
		SDL_DestroyTexture(texture);
	}

    SDL_QuitSubSystem(SDL_INIT_EVERYTHING);      
}


void AndroidUI::render_frame()
{
    SDL_RenderClear(renderer);
    SDL_UpdateTexture(texture, NULL, screen.data(),  sizeof(u32) * X);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);      
}

// okay make a printable monospace ASCII font
// and get a basic text rendering routine

// then we can try rendering "buttons" for traversing dirs and getting a file back
// we need to basically hook up something similar to imgui where can request back a file path for a specific purpose

// after this we can get the emulator ui for the horizontal configuration done
// and then do get one for the vertical configuration done

// we may want to generalize this onto a buffer so this primtive can be used in any frontend
// as it is very useful in any
void AndroidUI::draw_text(const std::string &str, u32 x, u32 y)
{
    assert(x < X);
    assert(y < Y);


    
}

void android_ui()
{
    AndroidUI window;

    for(;;)
    {

        SDL_Event event;

        // handle input
        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {


                case SDL_QUIT:
                {
                    return;
                }
            } 
        }


        window.draw_text("hello world!",X / 2, Y / 2);


        window.render_frame();     
    }
}