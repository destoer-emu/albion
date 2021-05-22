#include <n64/n64.h>

namespace nintendo64
{


using DISASS_FUNC = std::string (*)(N64 &n64,u64 addr);

// if i recall we just switch on the top 6 bits
// and the others if we hit a coprocessor opcode
// so simply enough we should just write something to gen a nice function table for us

// okay lets figure out a good way to decode these again
// and actually worry about the speed of it this time
std::string disass_opcode(N64 &n64,u64 addr)
{
    UNUSED(n64);
    printf("disass: unknown opcode at: %16zx\n",addr);
    exit(1);

    return "";
}

}