#include <n64/n64.h>

namespace nintendo64
{

static constexpr u32 CONFIG_FLAG = 1 << 0;
static constexpr u32 CHALLENGE_FLAG = 1 << 1;
static constexpr u32 TERMINATE_FLAG = 1 << 3;
static constexpr u32 LOCKOUT_FLAG = 1 << 4;
static constexpr u32 AQUIRE_CHECKSUM_FLAG = 1 << 5;
static constexpr u32 RUN_CHECKSUM_FLAG =  1 << 6;
static constexpr u32 ACK_FLAG = 1 << 7;

//https://www.qwertymodo.com/hardware-projects/n64/n64-controller
void controller_info(N64& n64)
{
    auto& mem = n64.mem;

    // controller
    handle_write_n64<u16>(mem.pif_ram,0,0x0500);

    // for now no pack
    handle_write_n64<u8>(mem.pif_ram,2,0x2);
}

void controller_state(N64& n64)
{
    auto& mem = n64.mem;

    //printf("state : %x\n",mem.joybus.state);

    handle_write_n64<u32>(mem.pif_ram,0,mem.joybus.state);
}

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

void joybus_comands(N64& n64)
{
    auto& mem = n64.mem;
    auto& joybus = mem.joybus;

    //printf("joybus command: %x\n",joybus.command);

    switch(joybus.command)
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
            printf("unknown joybus command: %x\n",joybus.command);
            break;            
        }
    }
}

void handle_pif_commands(N64& n64)
{
    const u8 commands = handle_read_n64<u8>(n64.mem.pif_ram,PIF_MASK);

    // joybus config
    if(commands & CONFIG_FLAG)
    {
        auto& joybus = n64.mem.joybus;
        joybus.command = handle_read_n64<u8>(n64.mem.pif_ram,0x0);
        n64.mem.joybus_enabled = true;
    }

    if(commands & CHALLENGE_FLAG)
    {
        printf("pif: cic channlge\n");
    }

    if(commands & TERMINATE_FLAG)
    {
        printf("pif: terminate\n");
    }

    if(commands & LOCKOUT_FLAG)
    {
        printf("pif: lockout\n");
    }

    if(commands & AQUIRE_CHECKSUM_FLAG)
    {
        printf("pif: aquire checksum\n");
    }

    if(commands & RUN_CHECKSUM_FLAG)
    {
        printf("pif: run checksum\n");
    }

    handle_write_n64<u8>(n64.mem.pif_ram,PIF_MASK,0);
}


template<typename access_type>
void write_pif(N64& n64, u64 addr, access_type v)
{
    const u64 offset = addr & PIF_MASK;

    handle_write_n64<access_type>(n64.mem.pif_ram,offset,v);

    // command byte written into last byte
    if(offset + sizeof(access_type) == PIF_SIZE)
    {
        handle_pif_commands(n64);
    }
}

template<typename access_type>
access_type read_pif(N64& n64, u64 addr)
{
    const u64 offset = addr & PIF_MASK;

    return handle_read_n64<access_type>(n64.mem.pif_ram,offset);
}

}