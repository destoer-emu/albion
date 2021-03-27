#pragma once
#include <destoer-emu/debug.h>
#include <destoer-emu/lib.h>
#include <gb/forward_def.h>


namespace gameboy
{

class GBDebug : public Debug 
{
public:
    GBDebug(GB &gb);


    enum class arg_type
    {
        string,
        integer
    };

    struct CommandArg
    {
        CommandArg(const std::string &l,arg_type t) : literal(l), type(t)
        {

        }

        std::string literal;
        arg_type type;
    };



    template<typename... Args>
    void print_console(std::string x,Args... args)
    {
        // assume SDL for now
        const auto str = fmt::format(x,args...);
        std::cout << str;
    }


    void debug_input();
    void execute_command(const std::string &command, const std::vector<CommandArg> &args);



    bool process_args(const std::string &line,std::vector<CommandArg> &args, std::string &command);

    // standard commands
    void breakpoint(const std::vector<CommandArg> &args);
    void set_break_internal(const std::vector<CommandArg> &args, bool watch);
    void watch(const std::vector<CommandArg> &args);
    void enable_watch(const std::vector<CommandArg> &args);
    void disable_watch(const std::vector<CommandArg> &args);
    void list_watchpoint(const std::vector<CommandArg> &args);
    void run(const std::vector<CommandArg> &args);
    void regs(const std::vector<CommandArg> &args);
    void step(const std::vector<CommandArg> &args);
    void disass(const std::vector<CommandArg> &args);
    void print_trace(const std::vector<CommandArg> &args);
    void print_mem(const std::vector<CommandArg> &args);
    void clear_breakpoint(const std::vector<CommandArg> &args);
    void enable_breakpoint(const std::vector<CommandArg> &args);
    void disable_breakpoint(const std::vector<CommandArg> &args);    
    void list_breakpoint(const std::vector<CommandArg> &args);
    void print_breakpoint(const Breakpoint &b);
    


private:
    using COMMAND_FUNC =  void (GBDebug::*)(const std::vector<CommandArg>&);
    std::unordered_map<std::string,COMMAND_FUNC> func_table =
    {
        {"break",&GBDebug::breakpoint},
        {"run", &GBDebug::run},
        {"regs",&GBDebug::regs},
        {"step",&GBDebug::step},
        {"disass",&GBDebug::disass},
        {"trace",&GBDebug::print_trace},
        {"mem",&GBDebug::print_mem},
        {"break_clear",&GBDebug::clear_breakpoint},
        {"break_enable",&GBDebug::enable_breakpoint},
        {"break_disable",&GBDebug::disable_breakpoint},
        {"break_list",&GBDebug::list_breakpoint},
        {"watch",&GBDebug::watch},
        {"watch_enable",&GBDebug::enable_watch},
        {"watch_disable",&GBDebug::disable_watch},
        {"watch_list",&GBDebug::list_watchpoint}
    };

    GB &gb;
    bool quit;
};

}