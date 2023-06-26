#include <n64/n64.h>

#include <n64/cpu/cop0.cpp>
#include <n64/cpu/cop1.cpp>

namespace nintendo64
{

std::string disass_n64(N64& n64, Opcode opcode, u64 addr)
{
    return beyond_all_repair::disass_mips(n64.program,addr,opcode);
}

template<const b32 debug>
void call_handler(N64& n64, const Opcode& opcode, u32 idx)
{
    if constexpr(debug)
    {
        INSTR_TABLE_DEBUG[idx](n64,opcode);
    }

    else
    {
        INSTR_TABLE_NO_DEBUG[idx](n64,opcode);
    }    
}

void reset_cpu(N64 &n64)
{
    auto &cpu = n64.cpu;

    // setup regs to hle the pif rom
    memset(cpu.regs,0,sizeof(cpu.regs));
    cpu.regs[T3] = 0xFFFFFFFFA4000040;
    cpu.regs[S4] = 0x0000000000000001;
    cpu.regs[S6] = 0x000000000000003F;
    cpu.regs[SP] = 0xFFFFFFFFA4001FF0;

    auto& cop0 = n64.cpu.cop0;

    cop0.random = 0x0000001F;
    write_cop0(n64,0x70400004,STATUS);
    write_cop0(n64,0x00000B00,PRID);
    write_cop0(n64,0x0006E463,CONFIG);
    write_cop0(n64,0,COUNT);
    write_cop0(n64,0,COMPARE);

    cpu.pc = 0xA4000040;
    cpu.pc_next = cpu.pc + 4; 

    insert_count_event(n64);
}


void cycle_tick(N64 &n64, u32 cycles)
{
    n64.scheduler.delay_tick(cycles);
}




template<const b32 debug>
void step(N64 &n64)
{
    const u32 op = read_u32<debug>(n64,n64.cpu.pc);

// TODO: this needs to optimised
#ifdef DEBUG 
    if constexpr(debug)
    {
        if(n64.debug.breakpoint_hit(u32(n64.cpu.pc),op,break_type::execute))
        {
            // halt until told otherwhise :)
            write_log(n64.debug,"[DEBUG] execute breakpoint hit ({:x}:{:x})",n64.cpu.pc,op);
            n64.debug.halt();
            return;
        }
    }    
#endif

    const Opcode opcode = beyond_all_repair::make_opcode(op);

    //std::cout << std::format("{:16x}: {}\n",n64.cpu.pc,disass_n64(n64,op,n64.cpu.pc_next));
    
    skip_instr(n64.cpu);

    // call the instr handler
    //const u32 offset = beyond_all_repair::get_opcode_type(opcode.op);
    const u32 offset = beyond_all_repair::calc_base_table_offset(opcode);
    
    call_handler<debug>(n64,opcode,offset);
    
    // $zero is hardwired to zero, make sure writes cant touch it
    n64.cpu.regs[R0] = 0;

    // assume 2 CPI
    cycle_tick(n64,2);
}

void write_pc(N64 &n64, u64 pc)
{
    if((pc & 0b11) != 0)
    {
        unimplemented("pc address exception");
    }

    n64.debug.trace.add(n64.cpu.pc,pc);	

    n64.cpu.pc_next = pc;
}

void skip_instr(Cpu &cpu)
{
    cpu.pc = cpu.pc_next;
    cpu.pc_next += 4;
}


}