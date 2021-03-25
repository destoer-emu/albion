#include <destoer-emu/debug.h>

#ifdef DEBUG


// wake the instance up
void Debug::wake_up()
{
    halted = false;
}

// halt the instance so the debugger is free to poke at it
void Debug::halt()
{
    halted = true;
}

//disable any form of breakpoint
void Debug::disable_everything()
{
    wake_up();
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

    bool hit =  b.is_hit(addr,type,value);


    if(hit)
    {
        printf("%x breakpoint hit at %x:%x\n",static_cast<int>(type),addr,value);
    }

    return hit;
}

void Debug::set_breakpoint(uint32_t addr,bool r, bool w, bool x, bool value_enabled, uint32_t value)
{
    Breakpoint b;

    b.set(addr,r,w,x,value_enabled,value,true);

    breakpoints[addr] = b;
}


void Breakpoint::set(uint32_t Addr, bool r, bool w, bool x, 
    bool Value_enabled,uint32_t Value,bool Break_enabled)
{
    value = Value;
    addr = Addr;
    break_enabled = Break_enabled;
    value_enabled = Value_enabled;

    break_setting = 0;

    if(r)
    {
        break_setting |= static_cast<int>(break_type::read);
    }

    if(w)
    {
        break_setting |= static_cast<int>(break_type::write);
    }

    if(x)
    {
        break_setting |= static_cast<int>(break_type::execute);
    }

}

void Breakpoint::disable()
{
    break_enabled = false;
}

void Breakpoint::enable()
{
    break_enabled = true;
}

// breakpoints are broken
bool Breakpoint::is_hit(uint32_t Addr,break_type type,uint32_t Value)
{
    // if the its not enabled or the value does not match if enabled
    // then it is not hit
    if(!break_enabled || (value_enabled && value != Value))
    {
        return false;
    }

    // if the type the breakpoint has been triggered for
    // is not being watched then we aernt interested
    if((static_cast<int>(type) & break_setting) == 0)
    {
        return false;
    }

    // in many cases this will be checked be callee
    // aswell can we optimise this?
    return addr == Addr;
}


Debug::Debug()
{
    log_file.open("emu.log");
    if(!log_file)
    {
        puts("failed to open log file!");
        exit(1);
    }
    disable_everything();
}

Debug::~Debug()
{
    log_file.close();
}
#endif