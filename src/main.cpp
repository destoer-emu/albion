#include "qt/qt_window.h"
#include "sdl/sdl_window.h"
#include "imgui_window.h"


int main(int argc, char *argv[])
{
#ifdef FRONTEND_QT    
    QApplication app(argc, argv); // assume a qt build for now..

    QtMainWindow window;

    window.setWindowTitle("destoer-emu: no rom");
    window.show();

    return app.exec();
#endif

#ifdef FRONTEND_SDL
    
    if(argc != 2)
    {
        printf("usage: %s <rom_name>\n",argv[0]);
        return 0;
    }

    SDL_SetMainReady();
    SDLMainWindow window(argv[1]);
#endif

#ifdef FRONTEND_IMGUI

    UNUSED(argc); UNUSED(argv);

    // sdl required for audio
    SDL_SetMainReady();
	if(SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		printf("Unable to initialize SDL: %s\n", SDL_GetError());
		return 1;
	}

    ImguiMainWindow window;

    window.mainloop();
#endif

    return 0;
}
