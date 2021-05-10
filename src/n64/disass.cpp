#include <n64/n64.h>

namespace nintendo64
{

std::string disass_opcode(N64 &n64,u64 addr)
{
    UNUSED(n64);
    printf("disass: unknown opcode at: %16zx\n",addr);
    exit(1);

    return "";
}

}