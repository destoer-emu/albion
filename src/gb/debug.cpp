#include <gb/gb.h>


// TODO: watchpoints will override breakpoints
// do we need a seperate map (i would rather not have two lookups as they are slow)
// do we need to roll our own address decoder to store entries?

#ifdef DEBUG
namespace gameboy
{

GBDebug::GBDebug(GB &g) : gb(g)
{

}

void GBDebug::execute_command(const std::vector<Token> &args)
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

// for use under SDL i dont know how we want to do the one for imgui yet...
void GBDebug::debug_input()
{
    print_console("{:4x}\n",gb.cpu.read_pc());
    std::string line = "";


    std::vector<Token> args;
    quit = false;
    while(!quit)
    {
        print_console("$ ");
        std::getline(std::cin,line);

        // lex the line and pull the command name along with the args.
        if(!tokenize(line,args))
        {
            // TODO: provide better error reporting
            print_console("one or more args is invalid");
        }
        
        execute_command(args);
        std::cin.clear();
    }
}

// these are better off being completly overriden
void GBDebug::regs(const std::vector<Token> &args)
{
    UNUSED(args);
    auto &cpu = gb.cpu;
    print_console("CPU REGS\n\nPC:{:04x}\nAF:{:04x}\nBC:{:04x}\nDE:{:04x}\nHL:{:04x}\nSP:{:04x}\n"
            ,cpu.read_pc(),cpu.read_af(),cpu.read_bc(),cpu.read_de(),cpu.read_hl(),cpu.read_sp());

    print_console("\nFLAGS\nC: {}\nH: {}\nN: {}\nZ: {}\n"
        ,cpu.read_flag_c(),cpu.read_flag_h(),cpu.read_flag_n(),cpu. read_flag_z());
}

void GBDebug::step_internal()
{
    gb.cpu.exec_instr_no_debug();
    halt();
}

uint32_t GBDebug::get_pc()
{
    return gb.cpu.read_pc();
}

void GBDebug::step(const std::vector<Token> &args)
{
    UNUSED(args);
    print_console("{:4x}: {}\n",gb.cpu.read_pc(),gb.disass.disass_op(gb.cpu.read_pc()));
    step_internal();
}


std::string GBDebug::disass_instr(uint32_t addr)
{
    uint32_t bank = addr < 0x8000? gb.mem.get_bank() : 0;
    return fmt::format("{:x}:{:x} {}",bank,addr,gb.disass.disass_op(addr));    
}

uint32_t GBDebug::get_instr_size(uint32_t addr)
{
    return gb.disass.get_op_sz(addr);
}

uint8_t GBDebug::read_mem(uint32_t addr)
{
    return gb.mem.raw_read(addr);
}

void GBDebug::change_breakpoint_enable(bool enable)
{
    gb.change_breakpoint_enable(enable);
}

}

#endif