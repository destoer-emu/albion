#include <n64/n64.h>

namespace nintendo64
{


void set_button(Joybus& joybus, u32 bit, b32 down)
{
    if(down)
    {
        joybus.state = set_bit(joybus.state,bit);
    }

    else
    {
        joybus.state = deset_bit(joybus.state,bit);
    }
}

void handle_input(N64& n64, Controller& controller)
{
    auto& joybus = n64.mem.joybus;

	for(auto& event : controller.input_events)
	{
		switch(event.input)
		{
            // a button
			case controller_input::a:
            {
                set_button(joybus,31,event.down);
                break;
            }

            // b button
			case controller_input::x: 
            {
                set_button(joybus,30,event.down);
                break;
            }

			default: break;
		}
    }


    u8 x = controller.left.x / 128;
    u8 y = controller.left.y / 128;        
    

    joybus.state = joybus.state & 0xffff'0000;
    joybus.state = joybus.state | (y << 0);
    joybus.state = joybus.state | (x << 8);
}

static constexpr u32 COMMAND_INFO = 0x00;
static constexpr u32 COMMAND_CONTROLLER_STATE = 0x01;
static constexpr u32 COMMAND_RESET = 0xff;

//https://www.qwertymodo.com/hardware-projects/n64/n64-controller
void controller_info(N64& n64)
{
    auto& mem = n64.mem;
    const u32 command_offset = mem.joybus.command_offset;

    // controller
    handle_write_n64<u16>(mem.pif_ram,command_offset + 0,0x0500);

    // for now no pack
    handle_write_n64<u8>(mem.pif_ram,command_offset + 2,0x2);
}

void controller_state(N64& n64)
{
    auto& mem = n64.mem;
    const u32 command_offset = mem.joybus.command_offset;

    //printf("state : %x\n",mem.joybus.state);

    handle_write_n64<u32>(mem.pif_ram,command_offset + 0,mem.joybus.state);
}


// http://en64.shoutwiki.com/wiki/SI_Registers_Detailed
void joybus_comands(N64& n64)
{
    auto& mem = n64.mem;
    auto& joybus = mem.joybus;


    b32 done = false;
    u32 i = 0;

    // TODO: this does not handle channels or errors...

    while(!done)
    {
        // how much will it write
        const u8 t = handle_read_n64<u8>(mem.pif_ram,i);

        // skip a byte
        if(t >= 0x80)
        {
            i++;
            continue;
        }

        const u8 r = handle_read_n64<u8>(mem.pif_ram,i + 1);

        const u8 command = handle_read_n64<u8>(mem.pif_ram,i + 2);

        joybus.command_offset = i + 3;

        switch(command)
        {
            // info
            case COMMAND_INFO:
            {
                controller_info(n64);
                break;
            }

            // controller state
            case COMMAND_CONTROLLER_STATE:
            {
                controller_state(n64);
                break;
            }

            case COMMAND_RESET:
            {
                // reset + info
                controller_info(n64);
                break;
            }

            default: 
            {
                //unimplemented("unknown joybus command: %x\n",command);
                break;            
            }
        }

        i += (3 + r + t);

        if(i >= PIF_SIZE)
        {
            done = true;
        }
    }
}

}