#include<gb/gb.h>


namespace gameboy
{

Scheduler::Scheduler(GB &gb) : cpu(gb.cpu), ppu(gb.ppu), 
    apu(gb.apu), mem(gb.mem)
{
    init();
}


void Scheduler::init()
{
    event_list.clear();
    timestamp = 0;
}



void Scheduler::service_event(const EventNode & node)
{
    const auto cycles_to_tick = timestamp - node.current;
    node.callback(cycles_to_tick);
}

void Scheduler::tick(uint32_t cycles)
{
    timestamp += cycles;

    // prevent overflows in our timestamps
    if(is_set(timestamp,(sizeof(timestamp)*8) - 1))
    {
        // find earliest timestamp start
        auto min = timestamp;

        for(const auto &x: event_list)
        {
            min = std::min(min,x.current);
        }

        // subtract from every timestamp
        for(auto &x: event_list)
        {
            x.current -= min;
            x.end -= min;
        }
        timestamp -= min;
    }

    for(auto it = event_list.begin(); it != event_list.end(); )
    {
        // if the timestamp is greater than the event fire
        // handle the event and remove it
        // we are not handling events that repeat atm
        if(timestamp >= it->end)
        {
            const auto event = *it;
            it = event_list.erase(it);
            service_event(event);
        }

        // as the list is sorted so the next event is first
        // any subsequent events aernt going to fire so return early
        else
        {
            break;
        }
    }
}

void Scheduler::insert(const EventNode &node,bool tick_old)
{
    remove(node.type,tick_old);

    // insert into the correct place
    auto found = std::find_if(event_list.begin(),event_list.end(),
    [&node](EventNode const &event)
    {
        return event.end >= node.end;
    });

    event_list.insert(found,node);
}

void Scheduler::remove(event_type type,bool tick_old)
{
    // insert into the correct place
    auto found = std::find_if(event_list.begin(),event_list.end(),
    [&type](EventNode const &event)
    {
        return event.type == type;
    });

    if(found != event_list.end())
    {
        const auto event = *found;
        event_list.erase(found);

        if(tick_old)
        {
            // smash off any accumluated cycles
            service_event(event);
        }
    }
}

uint32_t Scheduler::get_timestamp() const
{
    return timestamp;
}


EventNode Scheduler::create_event(uint32_t duration, event_type t,EventCallback func)
{
    return EventNode(timestamp,duration+timestamp,t,func);
}

// just because its convenient 
bool Scheduler::is_double() const
{
    return cpu.get_double();
}

}