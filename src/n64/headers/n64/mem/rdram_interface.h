namespace nintendo64
{

struct RdramInterface
{
    // RI
    u8 select = 0x14;
    u8 config = 0;
    u8 base = 0;
    u32 refresh = 0;    
};

}