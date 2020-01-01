#include "../headers/debug.h"


// think we need varidiac templates to achieve this the way we want it
#ifdef DEBUG

// wake the instance up
void Debug::wake_up()
{
    std::scoped_lock<std::mutex> guard(halt_mutex);
    halted = false;
}

// halt the instance so the debugger is free to poke at it
void Debug::halt()
{
    {
        std::scoped_lock<std::mutex> guard(halt_mutex);
        halted = true;
    }
    while(halted)
    {
        // repeatdadly sleep
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

//disable any form of breakpoint
void Debug::disable_everything()
{
    wake_up();
    step_instr = false;
    breakpoints_enabled = false;
}

bool Debug::breakpoint_hit(uint32_t addr, uint32_t value, break_type type)
{
    if(!breakpoints_enabled)
    {
        return false;
    }

    // map does not have a matching breakpoint
    if(!breakpoints.count(addr))
    {
        return false;
    }

    Breakpoint b = breakpoints[addr];

    return b.is_hit(addr,type,value);
}

void Debug::set_breakpoint(uint32_t addr,bool r, bool w, bool x, bool value_enabled, uint32_t value)
{
    Breakpoint b;

    b.set(addr,r,w,x,value_enabled,value,true);

    breakpoints[addr] = b;
}


void Breakpoint::set(uint32_t addr, bool r, bool w, bool x, 
    bool value_enabled,uint32_t value,bool break_enabled)
{
    this->value = value;
    this->addr = addr;
    this->break_enabled = break_enabled;
    this->value_enabled = value_enabled;
    this->r = r;
    this->w = w;
    this->x = x;
}

void Breakpoint::disable()
{
    break_enabled = false;
}

void Breakpoint::enable()
{
    break_enabled = true;
}

bool Breakpoint::is_hit(uint32_t addr,break_type type,uint32_t value)
{

    // if the its not enabled or the value does not match if enabled
    // then it is not hit
    if(!break_enabled || (value_enabled && this->value != value))
    {
        return false;
    }

    // if the type the breakpoint has been triggered for
    // is not being watched then we aernt interested
    switch(type)
    {
        case break_type::read:
        {
            if(!r)
            {
                return false;
            }
            break;
        }

        case break_type::write:
        {
            if(!w)
            {
                return false;
            }
            break;
        }

        case break_type::execute:
        {
            if(!x)
            { 
                return false;
            }
            break;
        }
    }

    // in many cases this will be checked be callee
    // aswell can we optimise this?
    return addr == this->addr;
}
#endif