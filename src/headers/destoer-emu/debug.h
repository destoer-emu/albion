#pragma once
#include <destoer-emu/lib.h>


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

    void add(uint64_t src, uint64_t dst)
    {
        history_source[idx] = src;
        history_target[idx] = dst;
        idx = (idx + 1) & 0xf;
    }

    std::string print()
    {
        std::string out = "pc trace:\n";
        for(int i = 0; i < 0x10; i++)
        {
            const auto offset = (idx + i) & 0xf;
            out += fmt::format("{}: {:8x} -> {:8x}\n",i,history_source[offset],history_target[offset]);
        }
        return out;
    }

    uint64_t idx;
    uint64_t history_target[0x10] = {0};
    uint64_t history_source[0x10] = {0};
};


enum class break_type : int
{
    
    write = 1 << 0,
    read = 1 << 1,
    execute = 1 << 2
};


// breakpoint helper class
struct Breakpoint
{
    void set(uint64_t addr, bool r, bool w, bool x, 
        bool value_enabled,uint64_t value,bool break_enabled, bool watch);

    void disable();

    void enable();

    bool is_hit(break_type type,uint64_t value);

    uint64_t value = 0xdeadbeef;
    bool value_enabled = false;
    bool break_enabled = false;
    uint64_t addr = 0xdeadbeef;
    int break_setting = 0;
    bool watch = false;
};





class Debug
{
public:

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


    template<typename... Args>
    void print_console(std::string x,Args... args)
    {
        // assume SDL for now
        const auto str = fmt::format(x,args...);
#ifdef FRONTEND_SDL
        std::cout << str;
#endif 

#ifdef FRONTEND_IMGUI
        console[console_idx] = str;
        console_idx = (console_idx + 1) & (console.size() - 1);
#endif
    }

#ifdef FRONTEND_IMGUI
    void draw_console();
#endif


#else
    void write_logger(std::string x,...) { UNUSED(x); }

    template<typename... Args>
    void print_console(std::string x,Args... args)
    {
        const auto str = fmt::format(x,args...);
        std::cout << str;
    }

#ifdef FRONTEND_IMGUI
    void draw_console() {}
#endif

#endif

    void breakpoint(const std::vector<Token> &args);
    void set_break_internal(const std::vector<Token> &args, bool watch);
    void watch(const std::vector<Token> &args);
    void enable_watch(const std::vector<Token> &args);
    void disable_watch(const std::vector<Token> &args);
    void list_watchpoint(const std::vector<Token> &args);
    void run(const std::vector<Token> &args);
    void print_trace(const std::vector<Token> &args);
    void print_mem(const std::vector<Token> &args);
    void clear_breakpoint(const std::vector<Token> &args);
    void enable_breakpoint(const std::vector<Token> &args);
    void disable_breakpoint(const std::vector<Token> &args);    
    void list_breakpoint(const std::vector<Token> &args);
    void print_breakpoint(const Breakpoint &b);
    void disass_internal(const std::vector<Token> &args);
    void debug_input();


    void wake_up();
    void halt();

    bool is_halted() const noexcept 
    {
        return halted;
    } 

    void disable_everything();


    bool breakpoint_hit(uint64_t addr, uint64_t value, break_type type);

    void set_breakpoint(uint64_t addr,bool r, bool w, bool x, bool value_enabled=false, uint64_t value=0xdeadbeef,bool watch = false);


    // map to hold breakpoints (lookup by addr)
    std::unordered_map<uint64_t,Breakpoint> breakpoints;

    bool breakpoints_enabled = false;
    bool watchpoints_enabled = false;
    bool log_enabled = false;


    Trace trace;

#ifdef FRONTEND_IMGUI
    std::vector<std::string> console;
    size_t console_idx = 0;
#endif

protected:
#ifdef DEBUG
    // internal overrides
    virtual void change_breakpoint_enable(bool enable) = 0;
    virtual uint8_t read_mem(uint64_t addr) = 0;
    virtual std::string disass_instr(uint64_t addr) = 0;
    virtual uint64_t get_instr_size(uint64_t addr) = 0;
    virtual void execute_command(const std::vector<Token> &args) = 0;
    virtual void step_internal() = 0;
    virtual uint64_t get_pc() = 0;
#endif

    std::ofstream log_file;
    bool log_full = false;
    // is debugged instance halted
    bool halted = false;    
    bool quit;
};



//----- logger macro definition ---
// might be a less nasty way to ensure these drop away
// when the debugger is not compiled into the code


#ifdef DEBUG
#define write_log(X,...) (X).write_logger(__VA_ARGS__)
#else 
#define write_log(X,...)
#endif