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

void GBADebug::execute_command(const std::string &command, const std::vector<CommandArg> &args)
{
    if(!func_table.count(command))
    {
        print_console("unknown command: '{}'\n",command);
        return;
    }

    const auto func = func_table[command];

    std::invoke(func,this,args);
}

// for use under SDL i dont know how we want to do the one for imgui yet...
void GBADebug::debug_input()
{
    print_console("{:8x}\n",gba.cpu.get_pc());
    std::string line = "";


    std::vector<CommandArg> args;
    std::string command = "";
    quit = false;
    while(!quit)
    {
        print_console("$ ");
        std::getline(std::cin,line);

        // lex the line and pull the command name along with the args.
        if(!process_args(line,args,command))
        {
            // TODO: provide better error reporting
            print_console("one or more args is invalid");
        }
        
        execute_command(command,args);
    }
}

// these are better off being completly overriden
void GBADebug::regs(const std::vector<CommandArg> &args)
{
    UNUSED(args);
    gba.cpu.print_regs();
}

void GBADebug::step(const std::vector<CommandArg> &args)
{
    UNUSED(args);
    const auto pc = gba.cpu.get_pc();
    const auto instr = gba.cpu.is_cpu_thumb()? gba.disass.disass_thumb(pc) : gba.disass.disass_arm(pc);
    print_console("{:8x}: {}\n",pc,instr);
    gba.cpu.exec_instr_no_debug();
}

std::string GBADebug::disass_instr(uint32_t addr)
{
    return disass_thumb? gba.disass.disass_thumb(addr) : gba.disass.disass_arm(addr);
}


void GBADebug::disassemble_arm(const std::vector<CommandArg> &args)
{
    UNUSED(args);
    disass_thumb = false;
    disass_internal(args);
}  

void GBADebug::disassemble_thumb(const std::vector<CommandArg> &args)
{
    UNUSED(args);
    disass_thumb = true;
    disass_internal(args);
}


uint32_t GBADebug::get_instr_size(uint32_t addr)
{
    UNUSED(addr);
    return (!disass_thumb * 2) + 2;
}

// TODO: use the no debug version when we have it
uint8_t GBADebug::read_mem(uint32_t addr)
{
    return gba.mem.read_mem<uint8_t>(addr);
}

void GBADebug::change_breakpoint_enable(bool enable)
{
    gba.change_breakpoint_enable(enable);
}

}

#endif