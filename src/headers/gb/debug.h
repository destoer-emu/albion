#pragma once
#include <destoer-emu/debug.h>
#include <destoer-emu/lib.h>
#include <gb/forward_def.h>


namespace gameboy
{
#ifdef DEBUG
struct GBDebug final : public Debug 
{
    GBDebug(GB &gb);


    // standard commands
    void regs(const std::vector<Token> &args);
    void step(const std::vector<Token> &args);
    void disass(const std::vector<Token> &args);    





    // overrides
    void change_breakpoint_enable(bool enable) override;
    uint8_t read_mem(uint64_t addr) override;
    std::string disass_instr(uint64_t addr) override;
    uint64_t get_instr_size(uint64_t addr) override;
    void execute_command(const std::vector<Token> &args) override;
    void step_internal() override;
    uint64_t get_pc() override;

    using COMMAND_FUNC =  void (GBDebug::*)(const std::vector<Token>&);
    std::unordered_map<std::string,COMMAND_FUNC> func_table =
    {
        {"break",&GBDebug::breakpoint},
        {"run", &GBDebug::run},
        {"regs",&GBDebug::regs},
        {"step",&GBDebug::step},
        {"disass",&GBDebug::disass_internal},
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
};

#else

struct GBDebug : public Debug 
{
    GBDebug(GB &gb) { UNUSED(gb); }
};

#endif
}