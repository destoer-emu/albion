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
void GBDebug::regs(const std::vector<Token> &args)
{
    UNUSED(args);
    auto &cpu = gb.cpu;
    print_console("CPU REGS\n\nPC:{:04x}\nAF:{:04x}\nBC:{:04x}\nDE:{:04x}\nHL:{:04x}\nSP:{:04x}\n"
            ,cpu.pc,cpu.read_af(),cpu.bc,cpu.de,cpu.hl,cpu.sp);

    print_console("\nFLAGS\nC: {}\nH: {}\nN: {}\nZ: {}\n"
        ,cpu.read_flag_c(),cpu.read_flag_h(),cpu.read_flag_n(),cpu. read_flag_z());
}

void GBDebug::step_internal()
{
    gb.cpu.exec_instr_no_debug();
    halt();
}



void GBDebug::step(const std::vector<Token> &args)
{
    UNUSED(args);
    print_console("{:4x}: {}\n",gb.cpu.pc,gb.disass.disass_op(gb.cpu.pc));
    step_internal();
}


std::string GBDebug::disass_instr(u64 addr)
{
    u64 bank = addr < 0x8000? gb.mem.cart_rom_bank : 0;
    return std::format("{:x}:{:x} {}",bank,addr,gb.disass.disass_op(addr));    
}

u64 GBDebug::get_instr_size(u64 addr)
{
    return gb.disass.get_op_sz(addr);
}

u8 GBDebug::read_mem(u64 addr)
{
    return gb.mem.raw_read(addr);
}

void GBDebug::write_mem(u64 addr, u8 v)
{
    gb.mem.raw_write(addr,v);
}

void GBDebug::change_breakpoint_enable(bool enable)
{
    gb.change_breakpoint_enable(enable);
}

b32 GBDebug::read_var(const std::string &name, u64* out)
{
    b32 success = true;

    if(name == "pc")
    {
        *out = gb.cpu.pc;
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