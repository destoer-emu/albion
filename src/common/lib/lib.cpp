#include <destoer-emu/lib.h>


void read_file(const std::string &filename, std::vector<uint8_t> &buf)
{
    std::ifstream fp(filename,std::ios::binary);

    if(!fp)
    {
        throw std::runtime_error(fmt::format("failed to open file: {}", filename));
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


std::vector<std::string> read_directory(const std::string &file_path)
{

    std::vector<std::string> dir_list;

    for(const auto &x: std::filesystem::directory_iterator(file_path))
    {
        dir_list.push_back(x.path().filename().string());
    }

    return dir_list;
}

// pull every file from every directory recursively
std::vector<std::string> get_dir_tree(const std::string &file_path)
{
    if(!std::filesystem::is_directory(file_path))
    {
        throw std::runtime_error(fmt::format("{} is not a directory",file_path));
    }


    std::vector<std::string> path_list;
    path_list.push_back(file_path);

    std::vector<std::string> tree;

    while(path_list.size() != 0)
    {
        auto dir = path_list.back(); path_list.pop_back();
        for(const auto &x: std::filesystem::directory_iterator(dir))
        {
            const auto path_str = x.path().string();

            if(std::filesystem::is_regular_file(x))
            {
                tree.push_back(path_str);
            }

            else if(std::filesystem::is_directory(x))
            {
                path_list.push_back(path_str);
            }
        }
    }

    return tree;
}


std::vector<std::string> filter_ext(const std::vector<std::string> &files,const std::string &str)
{
    std::vector<std::string> res;

    for(const auto &x : files)
    {
        size_t ext_idx = x.find_last_of("."); 
        if(ext_idx != std::string::npos)
        {
            std::string ext = x.substr(ext_idx+1);

            if(ext == str)
            {
                res.push_back(x);
            }

        }
    }

    return res;
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