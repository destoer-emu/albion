#pragma once
#ifdef FRONTEND_DESTOER

// this will have to be changed when this actually compiles to phone
#define SDL_MAIN_HANDLED
#ifdef _WIN32
#include <SDL.H>
#else
#include <SDL2/SDL.h>
#endif

void destoer_ui();

#endif