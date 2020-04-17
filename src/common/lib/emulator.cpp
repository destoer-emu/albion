#include <destoer-emu/emulator.h>

// for now we will just be lazy and detect by file ext
emu_type get_emulator_type(std::string filename)
{
	size_t ext_idx = filename.find_last_of("."); 
	if(ext_idx != std::string::npos)
	{
		std::string ext = filename.substr(ext_idx+1);

        if(ext == "gbc" || ext == "gb" || ext == "sgb")
        {
            return emu_type::gameboy;
        }

        else if(ext == "gba")
        {
            return emu_type::gba;
        }

        else
        {
            return emu_type::none;
        }

	}

    else 
    {
        return emu_type::none;
    }
}