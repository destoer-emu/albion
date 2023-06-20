#include <albion/lib.h>

void load_ips_patch(const std::string &filename,std::vector<uint8_t> &rom)
{
	auto fp = std::fstream(filename, std::ios::in | std::ios::binary);


    if(!fp)
    {
        return;
    }

    // we have ae ips file now read it in
    std::vector<uint8_t> buf;

    fp.seekg(0,std::ios::end);
    size_t size = fp.tellg();
    fp.seekg(0,std::ios::beg);

    fp.clear();


    buf.resize(size);

    fp.read((char*)buf.data(),size);
    fp.close();


    size_t idx = 0;
    // https://zerosoft.zophar.net/ips.php
    if(size > 5)
    {
        // skip header
        idx = 5;

        while(idx < size)
        {
            // record size
            if(idx + 5 >= size)
            {
                break;
            }

            uint32_t offset = 0;

            // read out 24bit value and bswap
            memcpy(&offset,&buf[idx],3);
            unsigned char *bswap_ptr = (unsigned char *)&offset;
            std::swap(bswap_ptr[0],bswap_ptr[2]);

            idx += 3;

            // eof
            if(offset == 0x464f45)
            {
                break;
            }


            const uint16_t len = bswap(handle_read<uint16_t>(buf,idx));
            idx += 2;

            // make sure our patching wont got out of bounds
            if(len + idx >= size || offset+len >= rom.size())
            {
                break;
            }

            // patch from buffer
            if(len != 0)
            {
                // patch byte
                for(int i = 0; i < len; i++)
                {
                    rom[offset++] = buf[idx++];
                }
            }

            // RLE
            else 
            {
                // extended record
                if(idx + 3 >= size)
                {
                    break;
                }


                const uint16_t rle_len = bswap(handle_read<uint16_t>(buf,idx));
                idx += 2;
                const uint8_t data = buf[idx++];

                if(offset + rle_len >= rom.size())
                {
                    break;
                }

                for(int i = 0; i < rle_len; i++)
                {
                    rom[offset+i] = data;
                }
            }
        }
    }
    
}

// dir helpers taken from old lib

constexpr char path_seperator = std::filesystem::path::preferred_separator;

void sort_alphabetically(std::vector<std::string> &vec)
{
    std::sort(vec.begin(), vec.end(), [](const auto& x, auto& y)
    {
        if(!x.size())
        {
            return false;
        }

        if(!y.size())
        {
            return true;
        }

        char x_char = tolower(x[0]);
        char y_char = tolower(y[0]);
        return x_char < y_char;
    });    
}



std::vector<std::string> read_directory(const std::string &file_path)
{

    std::vector<std::string> dir_list;

    for(const auto &x: std::filesystem::directory_iterator(file_path))
    {
        dir_list.push_back(x.path().filename().string());
    }

    return dir_list;
}

std::vector<std::string> read_sorted_directory(const std::string &file_path)
{
    auto dir_list = read_directory(file_path);

    // sort by file name
    sort_alphabetically(dir_list);

    // add back the directory path
    for(auto &x : dir_list)
    {
        x = file_path + path_seperator + x;
    }

    return dir_list;    
}