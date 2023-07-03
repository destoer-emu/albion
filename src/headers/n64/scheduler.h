#pragma once
#pragma once
#include <n64/forward_def.h>
#include <albion/lib.h>
#include <albion/debug.h>
#include <albion/scheduler.h>

namespace nintendo64
{

// just easy to put here
enum class n64_event
{
    line_inc,
    count,
    ai_dma,
    si_dma,
};

constexpr size_t EVENT_SIZE = 4;

struct N64Scheduler final : public Scheduler<EVENT_SIZE,n64_event>
{
    N64Scheduler(N64 &n) : n64(n)
    {

    }

    void skip_to_event();

    N64 &n64;
protected:
    void service_event(const EventNode<n64_event> & node) override;
};

}