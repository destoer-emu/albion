
namespace nintendo64
{

struct Joybus
{
    u32 command_offset = 0;

    // controller
    u32 state = 0;

    b32 enabled = false;
};

}