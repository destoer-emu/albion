#include <gba/gba.h>


// TODO: watchpoints will override breakpoints
// do we need a seperate map (i would rather not have two lookups as they are slow)
// do we need to roll our own address decoder to store entries?

#ifdef DEBUG
namespace gameboyadvance
{

GBADebug::GBADebug(GBA &g) : gba(g)
{

}

void GBADebug::execute_command(const std::vector<Token> &args)
{
    if(invalid_command(args))
    {
        print_console("invalid command\n");
        return;
    }

    const auto command = std::get<std::string>(args[0]);
    if(!func_table.count(command))
    {
        print_console("unknown command: '{}'\n",command);
        return;
    }

    const auto func = func_table[command];

    std::invoke(func,this,args);
}

// these are better off being completly overriden
void GBADebug::regs(const std::vector<Token> &args)
{
    UNUSED(args);
    gba.cpu.print_regs();
}

void GBADebug::step_internal()
{
    gba.cpu.exec_instr_no_debug();
    halt();
}



void GBADebug::step(const std::vector<Token> &args)
{
    UNUSED(args);
    const auto pc = gba.cpu.pc_actual;
    const auto instr = gba.cpu.is_thumb? gba.disass.disass_thumb(pc) : gba.disass.disass_arm(pc);
    print_console("{:8x}: {}\n",pc,instr);
    step_internal();
}

std::string GBADebug::disass_instr(uint64_t addr)
{
    return fmt::format("{:x}: {}",addr,
        disass_thumb? gba.disass.disass_thumb(addr) : gba.disass.disass_arm(addr));
}


void GBADebug::disassemble_arm(const std::vector<Token> &args)
{
    UNUSED(args);
    disass_thumb = false;
    disass_internal(args);
}  

void GBADebug::disassemble_thumb(const std::vector<Token> &args)
{
    UNUSED(args);
    disass_thumb = true;
    disass_internal(args);
}


void GBADebug::disass(const std::vector<Token> &args)
{
    UNUSED(args);
    disass_thumb = gba.cpu.is_thumb;
    disass_internal(args);
}

uint64_t GBADebug::get_instr_size(uint64_t addr)
{
    UNUSED(addr);
    return (!disass_thumb * 2) + 2;
}

uint8_t GBADebug::read_mem(uint64_t addr)
{
    return gba.mem.read_mem<uint8_t>(addr);
}

void GBADebug::write_mem(uint64_t addr, u8 v)
{
    gba.mem.write_mem<uint8_t>(addr, v);
}

void GBADebug::change_breakpoint_enable(bool enable)
{
    gba.change_breakpoint_enable(enable);
}

b32 GBADebug::read_var(const std::string &name, u64* out)
{
    b32 success = true;

    if(name == "pc")
    {
        *out = gba.cpu.regs[PC];
    }

    else
    {
        *out = 0;
        success = false;
    }

    return success;
}

}

#endif