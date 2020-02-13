#include <gb/apu.h>


namespace gameboy
{

void Apu::save_state(std::ofstream &fp)
{
    c1.save_state(fp); c1.sweep_save_state(fp);
    c2.save_state(fp);
    c3.save_state(fp);
    c4.save_state(fp);

	file_write_var(fp,sound_enabled);
	file_write_var(fp,is_double);
	file_write_arr(fp,audio_buf,sizeof(audio_buf));
	file_write_var(fp,audio_buf_idx); 
	file_write_var(fp,down_sample_cnt);
}


void Channel::chan_save_state(std::ofstream &fp)
{
    file_write_var(fp,lengthc);
    file_write_var(fp,length_enabled);
	file_write_var(fp,output);
}

void FreqReg::freq_save_state(std::ofstream &fp)
{
	file_write_var(fp,freq);
	file_write_var(fp,period);
    file_write_var(fp,duty_idx);
}

void Envelope::env_save_state(std::ofstream &fp)
{
	file_write_var(fp,env_period); 
	file_write_var(fp,env_load); 
	file_write_var(fp,volume);
	file_write_var(fp,volume_load);
	file_write_var(fp,env_enabled);
	file_write_var(fp,env_up);
}


void Square::save_state(std::ofstream &fp)
{
    file_write_var(fp,cur_duty);
}

void Wave::save_state(std::ofstream &fp)
{
	file_write_var(fp,volume);
	file_write_var(fp,volume_load);
}


void Noise::save_state(std::ofstream &fp)
{
	file_write_var(fp,clock_shift);
	file_write_var(fp,counter_width);
	file_write_var(fp,divisor_idx); 
	file_write_var(fp,shift_reg); 
	file_write_var(fp,period);
}

void Sweep::sweep_save_state(std::ofstream &fp)
{
	file_write_var(fp,sweep_enabled);
	file_write_var(fp,sweep_shadow);
	file_write_var(fp,sweep_period);
	file_write_var(fp,sweep_timer);
	file_write_var(fp,sweep_calced);
	file_write_var(fp,sweep_reg);
}



void Apu::load_state(std::ifstream &fp)
{
    c1.load_state(fp); c1.sweep_load_state(fp);
    c2.load_state(fp);
    c3.load_state(fp);
    c4.load_state(fp);


	file_read_var(fp,sound_enabled);
	file_read_var(fp,is_double);
	file_read_arr(fp,audio_buf,sizeof(audio_buf));
	file_read_var(fp,audio_buf_idx); 
	file_read_var(fp,down_sample_cnt);

    // prevent out of bound idx loads
    if(audio_buf_idx >= sample_size)
    {
        audio_buf_idx = 0;
    }
}



void Channel::chan_load_state(std::ifstream &fp)
{
    file_read_var(fp,lengthc);
    file_read_var(fp,length_enabled);
	file_read_var(fp,output);
}

void FreqReg::freq_load_state(std::ifstream &fp)
{
	file_read_var(fp,freq);
	file_read_var(fp,period);
    file_read_var(fp,duty_idx);
    duty_idx &= 7;
}

void Envelope::env_load_state(std::ifstream &fp)
{
	file_read_var(fp,env_period); 
	file_read_var(fp,env_load); 
	file_read_var(fp,volume);
	file_read_var(fp,volume_load);
	file_read_var(fp,env_enabled);
	file_read_var(fp,env_up);
}


void Square::load_state(std::ifstream &fp)
{
    file_read_var(fp,cur_duty);
    cur_duty &= 3;
}

void Wave::load_state(std::ifstream &fp)
{
	file_read_var(fp,volume);
	file_read_var(fp,volume_load);
}


void Noise::load_state(std::ifstream &fp)
{
	file_read_var(fp,clock_shift);
	file_read_var(fp,counter_width);
	file_read_var(fp,divisor_idx); 
	file_read_var(fp,shift_reg); 
	file_read_var(fp,period);
    divisor_idx &= 7;
}

void Sweep::sweep_load_state(std::ifstream &fp)
{
	file_read_var(fp,sweep_enabled);
	file_read_var(fp,sweep_shadow);
	file_read_var(fp,sweep_period);
	file_read_var(fp,sweep_timer);
	file_read_var(fp,sweep_calced);
	file_read_var(fp,sweep_reg);
}

}