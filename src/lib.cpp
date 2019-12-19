#include "headers/lib.h"
#include <fstream>


void read_file(std::string filename, std::vector<uint8_t> &buf)
{
    std::ifstream fp(filename,std::ios::binary);

    if(!fp)
    {
        std::cout << "failed to open file: " << filename;
        throw std::runtime_error("failed to open file");
    }



    fp.seekg(0,std::ios::end);
    size_t size = fp.tellg();
    fp.seekg(0,std::ios::beg);

    fp.clear();

    buf.resize(size);

    fp.read((char*)buf.data(),size);

}