
#include <n64/n64.h>

#ifdef DEBUG
namespace nintendo64
{

N64Debug::N64Debug(N64 &n) : n64(n)
{

}

void N64Debug::execute_command(const std::vector<Token> &args)
{
    if(!args.size())
    {
        print_console("empty command\n");
        return;
    }

    const auto command = args[0].literal;
    if(!func_table.count(command))
    {
        print_console("unknown command: '{}'\n",command);
        return;
    }

    const auto func = func_table[command];

    std::invoke(func,this,args);
}


// these are better off being completly overriden
void N64Debug::regs(const std::vector<Token> &args)
{
    UNUSED(args);
    //gba.cpu.print_regs();
}

void N64Debug::step_internal()
{
    nintendo64::step(n64);
    halt();
}

uint64_t N64Debug::get_pc()
{
    return n64.cpu.pc;
}

void N64Debug::step(const std::vector<Token> &args)
{
    UNUSED(args);
    const auto pc = get_pc();
    const auto instr = disass_instr(pc);
    print_console("{:8x}: {}\n",pc,instr);
    step_internal();
}

std::string N64Debug::disass_instr(uint64_t addr)
{
    return fmt::format("{:x}: {}",addr,"placeholder_str");
}


void N64Debug::disass(const std::vector<Token> &args)
{
    UNUSED(args);
    disass_internal(args);
}

uint64_t N64Debug::get_instr_size(uint64_t addr)
{
    UNUSED(addr);
    return 4;
}


uint8_t N64Debug::read_mem(uint64_t addr)
{
    //return gba.mem.read_mem<uint8_t>(addr);
    return addr;
}

void N64Debug::change_breakpoint_enable(bool enable)
{
    //gba.change_breakpoint_enable(enable);
    UNUSED(enable);
}

}
#endif