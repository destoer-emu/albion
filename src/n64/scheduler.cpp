#include <n64/n64.h>

namespace nintendo64
{

void N64Scheduler::service_event(const EventNode<n64_event> & node)
{
    switch(node.type)
    {
        case n64_event::line_inc:
        {
            increment_line(n64);
            break;
        }

        case n64_event::count:
        {
            count_intr(n64);
            break;
        }
    }
}

}