#include <gb/cpu.h>

namespace gameboy
{

void Cpu::save_state(std::ofstream &fp)
{
    file_write_var(fp,internal_timer);
    file_write_var(fp,joypad_state);
    file_write_var(fp,a);
    file_write_var(fp,f);
    file_write_var(fp,b);
    file_write_var(fp,c);
    file_write_var(fp,d);
    file_write_var(fp,e);
    file_write_var(fp,h);
    file_write_var(fp,l);
    file_write_var(fp,sp);
    file_write_var(fp,pc);
    file_write_var(fp,instr_side_effect);
    file_write_var(fp,interrupt_enable);
    file_write_var(fp,is_cgb);
    file_write_var(fp,is_double);
    file_write_var(fp,pending_cycles);
}


void Cpu::load_state(std::ifstream &fp)
{
    file_read_var(fp,internal_timer);
    file_read_var(fp,joypad_state);
    file_read_var(fp,a);
    file_read_var(fp,f);
    file_read_var(fp,b);
    file_read_var(fp,c);
    file_read_var(fp,d);
    file_read_var(fp,e);
    file_read_var(fp,h);
    file_read_var(fp,l);
    file_read_var(fp,sp);
    file_read_var(fp,pc);
    file_read_var(fp,instr_side_effect);
    if(instr_side_effect > instr_state::di || instr_side_effect < instr_state::normal)
    {
        throw std::runtime_error("load_state instr state");
    }
    file_read_var(fp,interrupt_enable);
    file_read_var(fp,is_cgb);
    file_read_var(fp,is_double);
    file_read_var(fp,pending_cycles);	
}

}