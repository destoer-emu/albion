namespace nintendo64
{
struct AudioInterface
{
    // status
    b32 full = false;
    b32 busy = false;
    b32 enabled = false;
};
}