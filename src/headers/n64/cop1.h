namespace nintendo64
{

struct Cop1
{
    // what are these supposed to be?
    u32 implementation = 5;
    u32 revision = 2;

    b32 fs = false;
    b32 c = false;
    u32 cause = 0;
    u32 flags = 0;
    u32 enable = 0;
    u32 rounding = 0;

    // TODO: need to read on there operations a bit more
    f64 regs[32] = {0.0};
};

}