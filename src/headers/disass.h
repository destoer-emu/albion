#include "forward_def.h"
#include "lib.h"


class Disass
{
public:
    void init(Memory *m);
    std::string disass_op(uint16_t addr);

private:
    Memory *mem;

};