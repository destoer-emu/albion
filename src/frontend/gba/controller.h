#pragma once
#include <gba/gba.h>
#include <frontend/controller.h>


class GbaControllerInput : public Controller
{
public:
    void update(gameboyadvance::GBA &gba);
};
