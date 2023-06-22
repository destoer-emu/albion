#include <albion/destoer-emu.h>
#include <gb/gb.h>
#include <font.h>
#include <pal.h>
// #include <gba/gba.h> GBA is not good enough to port yet it needs fire emblem fixed, and a HLE bios and probably a touch more optimisation

// okay lets get a basic android UI up and going
// Backed by SDL and rendered by hand

enum class current_window_t
{
    load_rom,
};

// base texture low res 16:9 we are going to let SDL just scale it
// atm this is large enough for any of the systems we have
static constexpr u32 SCREEN_WIDTH = 640;
static constexpr u32 SCREEN_HEIGHT = 360;

struct DestoerUI
{
    DestoerUI();
    ~DestoerUI();

    void render_frame();

    void draw();

    template<typename... Args>
    void draw_text(u32 x, u32 y, std::string fmt,Args... args);
    
    void draw_sprite(u32 x, u32 y, u32 sx, u32 xy, const u8 * src);

    template<typename... Args>
    void draw_button(u32 x, u32 y, u32 sx, u32 sy, std::string fmt,Args... args);

    current_window_t current_window;

	SDL_Window * window = NULL;
	SDL_Renderer * renderer = NULL;
	SDL_Texture * texture = NULL;

    bool is_horizontal;
    u32 display_x;
    u32 display_y;

    FpsCounter fps_counter;

    std::vector<u32> screen;    
};


DestoerUI::DestoerUI()
{

    // these should be pulled via SDL for now we are going to just fudge them so we can test it on the deskop
    const u32 width = SCREEN_WIDTH * 2;
    const u32 height = SCREEN_HEIGHT * 2;

    is_horizontal = width > height;

/*
    SDL_DisplayMode display_mode;

    SDL_GetCurrentDisplayMode(0,&display_mode);

    const u32 width = display_mode.w;
    const u32 height = display_mode.h;
*/

	// initialize our window
	window = SDL_CreateWindow("albion",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,width,height,SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
	
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl"); // crashes without this on windows?
	
	// set a render for our window
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	texture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_BGR888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

    screen.resize(SCREEN_WIDTH * SCREEN_HEIGHT);

    std::fill(screen.begin(),screen.end(),0xfffffff);    


    SDL_GL_SetSwapInterval(1);    
}

DestoerUI::~DestoerUI()
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


void DestoerUI::render_frame()
{
    SDL_RenderClear(renderer);
    SDL_UpdateTexture(texture, NULL, screen.data(),  sizeof(u32) * SCREEN_WIDTH);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);      
}

void clip_sprite(u32 x, u32 y, u32 &sx, u32 &sy)
{
    if(x + sx >= SCREEN_WIDTH)
    {
        sx = sx - ((x + sx) - SCREEN_WIDTH);
    }

    if(y + sy >= SCREEN_HEIGHT)
	{
		sy = sy - ((y + sy) - SCREEN_HEIGHT); 
	}
}

void DestoerUI::draw_sprite(u32 x, u32 y, u32 sx, u32 sy, const u8 * src)
{
	if(x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT)
	{
		return;
	}

	const u32 offset =  (y * SCREEN_WIDTH) + x;

    u32 sx_clipped = sx;
    u32 sy_clipped = sy;

	clip_sprite(x,y,sx_clipped,sy_clipped);

	for(u32 y_sprite = 0; y_sprite < sy_clipped; y_sprite++)
	{
		for(u32 x_sprite = 0; x_sprite < sx_clipped; x_sprite++)
		{
			const u32 pal_idx = src[(y_sprite * sx) + x_sprite];

            // if our pixels are transparent dont render
			if(pal_idx)
			{
				screen[offset + (y_sprite * SCREEN_WIDTH) + x_sprite] = PAL[pal_idx];
			}
		}
	}    
}

// then we can try rendering "buttons" for traversing dirs and getting a file back
// we need to basically hook up something similar to imgui where can request back a file path for a specific purpose

// after this we can get the emulator ui for the horizontal configuration done
// and then do get one for the vertical configuration done

// we may want to generalize this onto a buffer so this primtive can be used in any frontend
// as it is very useful in any

template<typename... Args>
void DestoerUI::draw_text(u32 x, u32 y, std::string fmt,Args... args)
{
    assert(x < SCREEN_WIDTH);
    assert(y < SCREEN_HEIGHT);

    const auto str = std::format(fmt,args...);

    // keep drawing 8 by 8 chars to the screen
    for(size_t i = 0; i < str.size(); i++)
    {
        const auto c = str[i];

        // printable ascii range
        if(c >= 0x20 && c <= 0x7f)
	    {
            const u32 font_offset = ((c - 0x20) * 64);

            draw_sprite(x + (i * 8) ,y,8,8,&FONT[font_offset]);
        }
    }
}

u32 centre_text_x(u32 x, u32 sx, u32 len)
{
    return x + ((sx / 2) - (len * 4));
} 

u32 centre_text_y(u32 y, u32 sy)
{
    return y + ((sy / 2) - 4);
}

template<typename... Args>
void DestoerUI::draw_button(u32 x, u32 y, u32 sx, u32 sy, std::string fmt,Args... args)
{
    assert(x < SCREEN_WIDTH);
    assert(y < SCREEN_HEIGHT);

    // draw a dark grey button
    // with a lighter grey outline


    u32 sx_clipped = sx;
    u32 sy_clipped = sy;

	clip_sprite(x,y,sx_clipped,sy_clipped);

    const u32 offset =  (y * SCREEN_WIDTH) + x;

    const u32 DARK_GRAY = 0xff404040;
    const u32 LIGHT_GRAY = 0xff606060;
    
	for(u32 y_sprite = 0; y_sprite < sy_clipped; y_sprite++)
	{
		for(u32 x_sprite = 0; x_sprite < sx_clipped; x_sprite++)
		{
			screen[offset + (y_sprite * SCREEN_WIDTH) + x_sprite] = DARK_GRAY;
		}
	}

    // draw seperator
    for(u32 x_sprite = 0; x_sprite < sx_clipped; x_sprite++)
    {
        screen[offset + (SCREEN_WIDTH * (sy_clipped - 1)) + x_sprite] = LIGHT_GRAY;
    }

    const auto str = std::format(fmt,args...);

    // draw the centre text 
    draw_text(centre_text_x(x,sx,str.size()), centre_text_y(y,sy),str);   
}

void DestoerUI::draw()
{
    // clear screen to black
    std::fill(screen.begin(),screen.end(),0xff000000);

    const std::string  menu_title = "LOAD ROM";
    draw_text(centre_text_x(0,SCREEN_WIDTH,menu_title.size()),0,menu_title);

    draw_button(0,8,SCREEN_WIDTH,20,"click me 1");
    draw_button(0,28,SCREEN_WIDTH,20,"click me 2");
}

void destoer_ui()
{
    DestoerUI window;

    for(;;)
    {
        window.fps_counter.reading_start();

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

        window.draw();
        window.render_frame();   

        window.fps_counter.reading_end();  
    }
}