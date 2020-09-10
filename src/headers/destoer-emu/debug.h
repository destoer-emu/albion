#pragma once
#include <destoer-emu/lib.h>

#ifdef DEBUG
enum class break_type : int
{
    
    write = 1 << 0,
    read = 1 << 1,
    execute = 1 << 2
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
    int break_setting = 0;
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

    bool is_halted() const noexcept 
    {
        return halted;
    } 

    void disable_everything();


    bool breakpoint_hit(uint32_t addr, uint32_t value, break_type type);

    void set_breakpoint(uint32_t addr,bool r, bool w, bool x, bool value_enabled=false, uint32_t value=0xdeadbeef);


    // map to hold breakpoints (lookup by addr)
    std::unordered_map<uint32_t,Breakpoint> breakpoints;

    bool breakpoints_enabled = true;
    bool log_enabled = false;
    
private:
    std::ofstream log_file;
    bool log_full = false;
    // is debugged instance halted
    bool halted = false;    
};



//----- logger macro definition ---
// might be a less nasty way to ensure these drop away
// when the debugger is not compiled into the code
#define write_log(X,...) (X).write_logger(__VA_ARGS__)


#else


class Debug 
{
public: 
    void write_logger(std::string x,...) { UNUSED(x); }
};
#define write_log(...)


#endif  