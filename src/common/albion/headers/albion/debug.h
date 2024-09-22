#pragma once
#include <albion/lib.h>
#include <iostream>


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

    void add(u64 src, u64 dst)
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

    u64 idx;
    u64 history_target[0x10] = {0};
    u64 history_source[0x10] = {0};
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
    void set(u64 addr, b32 r, b32 w, b32 x, 
        b32 value_enabled,u64 value,b32 break_enabled, b32 watch);

    void disable();

    void enable();

    b32 is_hit(break_type type,u64 value);

    u64 value = 0xdeadbeef;
    b32 value_enabled = false;
    b32 break_enabled = false;
    u64 addr = 0xdeadbeef;
    int break_setting = 0;
    b32 watch = false;
};




using Token = std::variant<u64, std::string>;

// NOTE: order has to be the same as the variant decl
enum class token_type
{
    u64_t,
    str_t,
};

// basic tokenizer
b32 tokenize(const std::string &line,std::vector<Token> &args);
u32 convert_imm(const std::string &imm);


// TODO: clean up lots of this ifdef chaining
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
            auto str = fmt::vformat(x,fmt::make_format_args(args...));
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
        const auto str = fmt::vformat(x,fmt::make_format_args(args...));
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
        const auto str = fmt::vformat(x,fmt::make_format_args(args...));
        std::cout << str;
    }

#ifdef FRONTEND_IMGUI
    void draw_console() {}
#endif

#endif
    b32 invalid_command(const std::vector<Token>& args);
    void breakpoint(const std::vector<Token> &args);
    void set_break_internal(const std::vector<Token> &args, b32 watch);
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
    b32 tokenize(const std::string &line,std::vector<Token> &tokens);


    void wake_up();
    void halt();

    u64 read_pc();

    b32 is_halted() const noexcept 
    {
        return halted;
    } 

    void disable_everything();

    b32 breakpoint_hit_internal(u64 addr, u64 value, break_type type);

    inline b32 breakpoint_hit(u64 addr, u64 value, break_type type)
    {
        if(!breakpoints_enabled && !watchpoints_enabled)
        {
            return false;
        }

        return breakpoint_hit_internal(addr,value,type);
    }

    void set_breakpoint(u64 addr,b32 r, b32 w, b32 x, b32 value_enabled=false, u64 value=0xdeadbeef,b32 watch = false);


    // map to hold breakpoints (lookup by addr)
    std::unordered_map<u64,Breakpoint> breakpoints;

    b32 breakpoints_enabled = false;
    b32 watchpoints_enabled = false;
    b32 log_enabled = false;


    Trace trace;

#ifdef FRONTEND_IMGUI
    std::vector<std::string> console;
    size_t console_idx = 0;
#endif

protected:
#ifdef DEBUG
    // internal overrides
    virtual std::string disass_instr(u64 addr) = 0;
    virtual u64 get_instr_size(u64 addr) = 0;
    virtual void execute_command(const std::vector<Token> &args) = 0;
    virtual void step_internal() = 0;

    // NOTE: this must have a $pc value to read for impl read_pc()
    virtual b32 read_var(const std::string& name, u64* value_out) = 0;
#endif

    std::ofstream log_file;
    b32 log_full = false;
    // is debugged instance halted
    b32 halted = false;    
    b32 quit = false;

    // public overrides
public:
    virtual void change_breakpoint_enable(b32 enable) = 0;
    virtual u8 read_mem(u64 addr) = 0;
    virtual void write_mem(u64 addr, u8 v) = 0;
};



//----- logger macro definition ---
// might be a less nasty way to ensure these drop away
// when the debugger is not compiled into the code


#ifdef DEBUG
#define write_log(X,...) (X).write_logger(__VA_ARGS__)
#else 
#define write_log(X,...)
#endif