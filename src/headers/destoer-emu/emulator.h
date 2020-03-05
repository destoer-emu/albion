#pragma once
#include <destoer-emu/lib.h>


enum class emu_type
{
    gameboy, gba
};

enum class emu_test
{
    running, pass, fail
};

emu_type get_emulator_type(std::string filename);