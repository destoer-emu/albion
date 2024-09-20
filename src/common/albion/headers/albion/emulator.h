#pragma once
#include <albion/lib.h>


enum class emu_type
{
    gameboy, gba, n64, none
};

enum class emu_test
{
    running, pass, fail
};

emu_type get_emulator_type(std::string filename);
std::string get_emulator_name(emu_type);