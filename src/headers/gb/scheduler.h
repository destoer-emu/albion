#pragma once
#include <gb/forward_def.h>
#include <destoer-emu/lib.h>
#include <destoer-emu/debug.h>

namespace gameboy
{


using EventCallback = std::function<void(int)>; 

struct EventNode
{
    EventNode(uint32_t c, uint32_t e, event_type t,EventCallback func)
    {
        current = c;
        end = e;
        type = t;
        callback = func;
    }

    // when event was added
    uint32_t current;

    // when it will trigger
    uint32_t end;

    event_type type;

    std::function<void(int)> callback;
};

/*
struct EventCmp
{

bool operator () (EventNode &a, EventNode &b) const
{
    return a.end < b.end;
}

};
*/

class Scheduler
{
public:

    bool is_double() const;

    void init();

    void tick(uint32_t cycles);

    Scheduler(GB &gb);

    void insert(const EventNode &node, bool tick_old=true);

    // remove events of the specifed type
    void remove(event_type type,bool tick_old=true);

    uint32_t get_timestamp() const;

    // helper to create events
    EventNode create_event(uint32_t duration, event_type t,EventCallback func);

private:
    void service_event(const EventNode & node);

    Cpu &cpu;
    Ppu &ppu;
    Apu &apu;
    Memory &mem;

    // need to switch this over to a heap 
    // at some point but this is fine just for testing
    // but we wont know if this is really faster
    // until we do 
    std::list<EventNode> event_list;

    // also might keep a seperate list of bools
    // that say if the event is even active
    // so skip deletion lookups if its not there
    // when we move over to a heap
    // we should probably store indexes to the event here
    // so we can quickly "reorder" a existing event
    bool active[EVENT_SIZE] = {false};

    // current elapsed time
    uint32_t timestamp = 0;
};

}