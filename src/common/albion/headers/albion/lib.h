#pragma once
#include <spdlog/fmt/bundled/format.h>
#include <spdlog/spdlog.h>
#include <destoer/destoer.h>
using namespace destoer;



void load_ips_patch(const std::string &filename,std::vector<uint8_t> &rom);

std::vector<std::string> read_sorted_directory(const std::string &file_path);


template<typename... Args>
inline std::string runtime_format(const char* fmt, Args... args)
{
    return fmt::vformat(fmt,fmt::make_format_args(args...));
}