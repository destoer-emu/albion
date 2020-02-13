#include <destoer-emu/lib.h>


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


std::vector<std::string> read_directory(std::string file_path)
{

    std::vector<std::string> dir_list;

    for(auto &x: std::filesystem::directory_iterator(file_path))
    {
        dir_list.push_back(x.path().filename().string());
    }

    return dir_list;
}



std::vector<std::string> read_sorted_directory(std::string file_path)
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