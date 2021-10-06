#include <gb/apu.h>



namespace gameboy_psg
{


}


namespace gameboy
{

void Apu::load_state(std::ifstream &fp)
{
	file_read_var(fp,down_sample_cnt);
	psg.load_state(fp);
}


void Apu::save_state(std::ofstream &fp)
{
	file_write_var(fp,down_sample_cnt);
	psg.save_state(fp);
}

}