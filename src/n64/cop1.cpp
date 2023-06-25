
namespace nintendo64
{

void check_cop1_exception(N64& n64)
{
    auto& cop1 = n64.cpu.cop1;

    if(cop1.enable & cop1.cause || is_set(cop1.cause,6))
    {
        // TODO: we need to look at exception handling
        // see the exception chapter... looks like FPE bit
        // and whatever the normal exception handling is...
        assert(false);
    }
}


void write_cop1_control(N64& n64, u32 idx, u32 v)
{ 
    auto& cop1 = n64.cpu.cop1;


    // only the control reg is writeable
    if(idx == 31)
    {
        cop1.fs = is_set(v,24);
        cop1.c = is_set(v,23);

        // cause is read only
        // dont write it

        cop1.enable = (v >> 7) & 0b111'11;
        cop1.flags = (v >> 2) & 0b111'11;
        cop1.rounding = v & 0b11;
        check_cop1_exception(n64);
    }
}

u32 read_cop1_control(N64& n64, u32 idx)
{
    auto& cop1 = n64.cpu.cop1;

    switch(idx)
    {
        case 31:
        { 
            return (cop1.fs << 24) | (cop1.c << 23) | (cop1.cause << 12) | 
            (cop1.enable << 7) | (cop1.flags << 2) | cop1.rounding;
        }

        case 0: 
        {
            return cop1.revision | (cop1.implementation);
        }
        
        default: return 0;
    }
}


}