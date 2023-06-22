
#include <n64/n64.h>

#ifdef DEBUG
namespace nintendo64
{

N64Debug::N64Debug(N64 &n) : n64(n)
{

}

void N64Debug::execute_command(const std::vector<Token> &args)
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
void N64Debug::regs(const std::vector<Token> &args)
{
    UNUSED(args);
    
    printf("pc = %016lx\n",n64.cpu.pc);

    for(u32 i = 0; i < 32; i++)
    {
        printf("%s = %016lx\n",reg_names[i],n64.cpu.regs[i]);
    }
}

void N64Debug::step_internal()
{
    const auto old = breakpoints_enabled;
    breakpoints_enabled = false;
    nintendo64::step(n64);
    breakpoints_enabled = old;
    halt();
}

void N64Debug::step(const std::vector<Token> &args)
{
    UNUSED(args);
    const auto pc = read_pc();
    const auto instr = disass_instr(pc);
    print_console("{}\n",instr);
    step_internal();
}

std::string N64Debug::disass_instr(u64 addr)
{
    const u32 opcode = read_u32(n64,addr);

    Opcode op;
    init_opcode(op,opcode);  

    return fmt::format("{:x}: {}",addr,disass_opcode(op,addr+sizeof(u32)));
}


void N64Debug::disass(const std::vector<Token> &args)
{
    UNUSED(args);
    disass_internal(args);
}

u64 N64Debug::get_instr_size(u64 addr)
{
    UNUSED(addr);
    return sizeof(u32);
}


u8 N64Debug::read_mem(u64 addr)
{
    return read_u8(n64,addr);
}

void N64Debug::write_mem(u64 addr, u8 v)
{
    write_u8(n64,addr,v);
}

void N64Debug::change_breakpoint_enable(bool enable)
{
    UNUSED(enable);
}

b32 N64Debug::read_var(const std::string &name, u64* out)
{
    b32 success = true;

    if(name == "pc")
    {
        *out = n64.cpu.pc;
    }

    else
    {
        // TODO: we could make this faster
        for(u32 i = 0; i < REG_NAMES_SIZE; i++)
        {
            if(reg_names[i] == name)
            {
                *out = n64.cpu.regs[i];
                return true;
            }
        }


        *out = 0;
        success = false;
    }

    return success;
}

}
#endif