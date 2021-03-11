#pragma once
#include <destoer-emu/lib.h>


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



    // logging function!
#ifdef DEBUG
    Debug();
    ~Debug();
    
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
#else
    void write_logger(std::string x,...) { UNUSED(x); }
#endif


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



struct Trace
{
    Trace()
    {
        clear();
    }

    void clear()
    {
        idx = 0;
        memset(history_target,0,sizeof(history_target));
        memset(history_source,0,sizeof(history_source));
    }

    void add(uint32_t src, uint32_t dst)
    {
        history_source[idx] = src;
        history_target[idx] = dst;
        idx = (idx + 1) & 0xf;
    }

    void print()
    {
        puts("pc trace:\n");
        for(int i = 0; i < 0x10; i++)
        {
            const auto offset = (idx + i) & 0xf;
            printf("%d: %08x -> %08x\n",i,history_source[offset],history_target[offset]);
        }
    }

    uint32_t idx;
    uint32_t history_target[0x10] = {0};
    uint32_t history_source[0x10] = {0};
};



//----- logger macro definition ---
// might be a less nasty way to ensure these drop away
// when the debugger is not compiled into the code
#define write_log(X,...) (X).write_logger(__VA_ARGS__)
