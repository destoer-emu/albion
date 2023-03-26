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