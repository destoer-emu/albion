#pragma once
#include <gb/gb.h>
#include <frontend/controller.h>


class GbControllerInput : public Controller
{
public:
    void update(gameboy::GB &gb);
};
