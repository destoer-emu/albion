#pragma once
#include<destoer-emu/min_heap.h>

// needs a save state impl
template<size_t EVENT_SIZE,typename event_type>
class Scheduler
{
public:
    void init();

    void save_state(std::ofstream &fp);
    void load_state(std::ifstream &fp);    

    void tick(uint32_t cycles);
    bool is_active(event_type t) const;

    std::optional<EventNode<event_type>> get(event_type t) const;
    std::optional<size_t> get_event_ticks(event_type t) const;

    void insert(const EventNode<event_type> &node, bool tick_old=true);

    // remove events of the specifed type
    void remove(event_type type,bool tick_old=true);

    uint64_t get_timestamp() const;

    uint64_t get_next_event_cycles() const;

    size_t size() const
    { 
        return event_list.size(); 
    } 

    // helper to create events
    EventNode<event_type> create_event(uint32_t duration, event_type t) const;

protected:
    virtual void service_event(const EventNode<event_type> & node) = 0;


    MinHeap<EVENT_SIZE,event_type> event_list;

    // current elapsed time
    uint32_t timestamp = 0;
};



template<size_t SIZE,typename event_type>
void Scheduler<SIZE,event_type>::init()
{
    event_list.clear();
    timestamp = 0;
}

template<size_t SIZE,typename event_type>
bool Scheduler<SIZE,event_type>::is_active(event_type t) const
{
    return event_list.is_active(t);
}

template<size_t SIZE,typename event_type>
void Scheduler<SIZE,event_type>::tick(uint32_t cycles)
{
    timestamp += cycles;

    if(is_set(timestamp,31))
    {
        uint32_t min = 0xffffffff;
        puts("timestamp overflow");
        for(const auto &x: event_list.buf)
        {
            if(event_list.is_active(x.type))
            {
                min = std::min(min,x.start);
            }
        }

        for(auto &x: event_list.buf)
        {
            if(event_list.is_active(x.type))
            {
                x.end -= min;
                x.start -= min;
            }
        }
        timestamp -= min;
    }

    while(event_list.size())
    {
        // if the timestamp is greater than the event fire
        // handle the event and remove it
        const auto event = event_list.peek();
        if(timestamp >= event.end)
        {
            // remove min event
            event_list.pop();
            service_event(event);
        }

        // as the list is sorted the next event is first
        // any subsequent events aernt going to fire so return early
        else
        {
            break;
        }
    }
}

template<size_t SIZE,typename event_type>
void Scheduler<SIZE,event_type>::insert(const EventNode<event_type> &node,bool tick_old)
{
    remove(node.type,tick_old);
    event_list.insert(node);
}

template<size_t SIZE,typename event_type>
void Scheduler<SIZE,event_type>::remove(event_type type,bool tick_old)
{
    const auto event = event_list.remove(type);

    // if it was removed and we want to tick off cycles
    if(event && tick_old)
    {
        service_event(event.value());
        event_list.remove(type);
    }
}

template<size_t SIZE,typename event_type>
std::optional<EventNode<event_type>> Scheduler<SIZE,event_type>::get(event_type t) const
{
    return event_list.get(t);
}

template<size_t SIZE,typename event_type>
std::optional<size_t> Scheduler<SIZE,event_type>::get_event_ticks(event_type t) const
{
    const auto event = get(t);

    if(!event)
    {
        return std::nullopt;
    }

    return timestamp - event.value().start;
}

template<size_t SIZE,typename event_type>
uint64_t Scheduler<SIZE,event_type>::get_timestamp() const
{
    return timestamp;
}

template<size_t SIZE,typename event_type>
uint64_t Scheduler<SIZE,event_type>::get_next_event_cycles() const
{
    return event_list.peek().end - timestamp;
}

template<size_t SIZE,typename event_type>
EventNode<event_type> Scheduler<SIZE,event_type>::create_event(uint32_t duration, event_type t) const
{
    return EventNode<event_type>(timestamp,duration+timestamp,t);
}

template<size_t SIZE,typename event_type>
void Scheduler<SIZE,event_type>::save_state(std::ofstream &fp)
{
    file_write_var(fp,timestamp);
    event_list.save_state(fp);
}

template<size_t SIZE,typename event_type>
void Scheduler<SIZE,event_type>::load_state(std::ifstream &fp)
{
    file_read_var(fp,timestamp);
    event_list.load_state(fp);
}