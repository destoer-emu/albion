#pragma once
#include "lib.h"

#ifdef DEBUG
enum class break_type
{
    write, read, execute
};


// breakpoint helper class
struct Breakpoint
{
    void set(uint32_t addr, bool r, bool w, bool x, 
        bool value_enabled,uint32_t value,bool break_enabled);

    void disable();

    void enable();

    bool is_hit(uint32_t addr,break_type type,uint32_t value);

    uint32_t value = 0xdeadbeef;
    bool value_enabled = false;
    bool break_enabled = false;
    uint32_t addr = 0xdeadbeef;
    bool x = false; // execute enable
    bool r = false; // read enable
    bool w = false; // write enabled
};


class Debug
{
public:

    // logging function!
    template<typename... Args>
    void write_logger(std::string x,Args... args)
    {
        if(log.size() > 3000)
        {
            if(!log_full)
            {
                log.push_back("Log limit reached!");
            }
            log_full = true;
        }

        else
        {
            log.push_back(fmt::format(x,args...));
            #ifdef LOG_CONSOLE
            std::cout << fmt::format(x,args...) << "\n";
            #endif
        }
    }

    const std::vector<std::string>& get_logs()
    { 
        return log;
    }

    void clear_logs()
    {
        log.clear();
    }

    void wake_up();
    void halt();

    void disable_everything();


    bool breakpoint_hit(uint32_t addr, uint32_t value, break_type type);

    void set_breakpoint(uint32_t addr,bool r, bool w, bool x, bool value_enabled=false, uint32_t value=0xdeadbeef);

    // step an instruction
    bool step_instr = false;

    // is debugged instance halted
    bool halted = false;

    // map to hold breakpoints (lookup by addr)
    std::map<uint16_t,Breakpoint> breakpoints;

    bool breakpoints_enabled = true;

    
private:
    std::vector<std::string> log;
    bool log_full = false;
};



//----- logger macro definition ---
// might be a less nasty way to ensure these drop away
// when the debugger is not compiled into the code
//#define write_log(text,...) debug->write_logger(text,__VA_ARGS__)
#define write_log(text,...)
#else
class Debug {};
#define write_log(text,...)

#endif  