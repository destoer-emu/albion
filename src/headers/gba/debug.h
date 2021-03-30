#pragma once
#include <destoer-emu/debug.h>
#include <destoer-emu/lib.h>
#include <gba/forward_def.h>


namespace gameboyadvance
{
#ifdef DEBUG
class GBADebug : public Debug 
{
public:
    GBADebug(GBA &gba);


    void debug_input();
    void execute_command(const std::string &command, const std::vector<CommandArg> &args);


    // standard commands
    void regs(const std::vector<CommandArg> &args);
    void step(const std::vector<CommandArg> &args);
    void disassemble_arm(const std::vector<CommandArg> &args);    
    void disassemble_thumb(const std::vector<CommandArg> &args);   





    // overrides
    void change_breakpoint_enable(bool enable) override;
    uint8_t read_mem(uint32_t addr) override;
    std::string disass_instr(uint32_t addr) override;
    uint32_t get_instr_size(uint32_t addr) override;

private:

    using COMMAND_FUNC =  void (GBADebug::*)(const std::vector<CommandArg>&);
    std::unordered_map<std::string,COMMAND_FUNC> func_table =
    {
        {"break",&GBADebug::breakpoint},
        {"run", &GBADebug::run},
        {"regs",&GBADebug::regs},
        {"step",&GBADebug::step},
        {"disass_thumb",&GBADebug::disassemble_thumb},
        {"disass_arm",&GBADebug::disassemble_arm},
        {"trace",&GBADebug::print_trace},
        {"mem",&GBADebug::print_mem},
        {"break_clear",&GBADebug::clear_breakpoint},
        {"break_enable",&GBADebug::enable_breakpoint},
        {"break_disable",&GBADebug::disable_breakpoint},
        {"break_list",&GBADebug::list_breakpoint},
        {"watch",&GBADebug::watch},
        {"watch_enable",&GBADebug::enable_watch},
        {"watch_disable",&GBADebug::disable_watch},
        {"watch_list",&GBADebug::list_watchpoint}
    };

    GBA &gba;
    bool disass_thumb = false;
};

#else

class GBADebug : public Debug 
{
public:
    GBADebug(GBA &gba) { UNUSED(gba); }
};

#endif
}