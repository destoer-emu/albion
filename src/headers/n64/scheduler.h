#pragma once
#pragma once
#include <n64/forward_def.h>
#include <destoer-emu/lib.h>
#include <destoer-emu/debug.h>
#include <destoer-emu/scheduler.h>

namespace nintendo64
{

// just easy to put here
enum class n64_event
{
    line_inc,
    count
};

constexpr size_t EVENT_SIZE = 2;

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