#pragma once
#include <destoer-emu/debug.h>
#include <destoer-emu/lib.h>
#include <gba/forward_def.h>


namespace gameboyadvance
{
#ifdef DEBUG
struct GBADebug final : public Debug 
{
    GBADebug(GBA &gba);


    // standard commands
    void regs(const std::vector<Token> &args);
    void step(const std::vector<Token> &args);
    void disassemble_arm(const std::vector<Token> &args);    
    void disassemble_thumb(const std::vector<Token> &args);   
    void disass(const std::vector<Token> &args);





    // overrides
    void change_breakpoint_enable(bool enable) override;
    uint8_t read_mem(uint64_t addr) override;
    std::string disass_instr(uint64_t addr) override;
    uint64_t get_instr_size(uint64_t addr) override;
    void execute_command(const std::vector<Token> &args) override;
    void step_internal() override;
    uint64_t get_pc() override;
    bool disass_thumb = false;
private:

    using COMMAND_FUNC =  void (GBADebug::*)(const std::vector<Token>&);
    std::unordered_map<std::string,COMMAND_FUNC> func_table =
    {
        {"break",&GBADebug::breakpoint},
        {"run", &GBADebug::run},
        {"regs",&GBADebug::regs},
        {"step",&GBADebug::step},
        {"disass",&GBADebug::disass},
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
};

#else

class GBADebug : public Debug 
{
public:
    GBADebug(GBA &gba) { UNUSED(gba); }
};

#endif
}