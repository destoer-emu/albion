#pragma once
#include <destoer.h>

enum class controller_input
{
    a,
    x,
    start ,
    select,
    right,
    left,
    up,
    down, 
    right_trigger,
    left_trigger,
};


struct InputEvent
{
    b32 down;
    controller_input input;
};

inline InputEvent make_input_event(controller_input input, b32 down)
{
	InputEvent event;

	event.down = down;
	event.input = input;

	return event;
}

struct Controller
{
    void add_event(const InputEvent& event)
    {
        input_events.push_back(event);
    }

    void add_event(controller_input input, b32 down)
    {
        const auto event = make_input_event(input,down);
        add_event(event);
    }

    // controller axis
    u32 left_x = 0;
    u32 left_y = 0;

    b32 simulate_dpad = true;

    std::vector<InputEvent> input_events;
};