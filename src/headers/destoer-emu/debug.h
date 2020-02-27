#pragma once
#include <destoer-emu/lib.h>

#ifdef DEBUG
enum class break_type
{
    write, read, execute
};


// breakpoint helper class
struct Breakpoint
{
    void set(uint32_t Addr, bool R, bool W, bool X, 
        bool Value_enabled,uint32_t Value,bool Break_enabled);

    void disable();

    void enable();

    bool is_hit(uint32_t Addr,break_type type,uint32_t Value);

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

    Debug();
    ~Debug();

    // logging function!
    template<typename... Args>
    void write_logger(std::string x,Args... args)
    {
        if(log_enabled)
        {
            auto str = fmt::format(x,args...);
            log_file << str << "\n";
            log_file.flush();
            #ifdef LOG_CONSOLE
            std::cout << str << "\n";
            #endif
        }
    }

    void wake_up();
    void halt();

    void disable_everything();


    bool breakpoint_hit(uint32_t addr, uint32_t value, break_type type);

    void set_breakpoint(uint32_t addr,bool r, bool w, bool x, bool value_enabled=false, uint32_t value=0xdeadbeef);

    // step an instruction
    bool step_instr = false;


    // map to hold breakpoints (lookup by addr)
    std::map<uint16_t,Breakpoint> breakpoints;

    bool breakpoints_enabled = true;
    bool log_enabled = true;
    
private:
    std::ofstream log_file;
    bool log_full = false;
    // is debugged instance halted
    bool halted = false;
    std::mutex halt_mutex;    
};



//----- logger macro definition ---
// might be a less nasty way to ensure these drop away
// when the debugger is not compiled into the code
#define write_log(...) debug->write_logger(__VA_ARGS__)


#else


class Debug 
{
public: 
    template<typename... Args>
    void write_logger(std::string x,Args... args) { UNUSED(x); }
};
#define write_log(...)


#endif  