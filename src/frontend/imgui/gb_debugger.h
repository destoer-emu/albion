#pragma once
#include "imgui_window.h"

void gameboy_handle_input(gameboy::GB &gb);
void gameboy_emu_instance(gameboy::GB &gb, Texture &screen);