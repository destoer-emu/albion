#pragma once
#include <spdlog/fmt/bundled/format.h>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>
#include <array>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <functional>
#include <exception>
#include <filesystem>
#include <numeric>
#include <limits>
#include <thread>
#include <mutex>
#include <chrono>
#include <map>
#include <list>
#include <set>
#include <fstream>
#include <type_traits>
#include <atomic>
#include <optional>
#include <variant>
#include <cstdint>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <climits>
#include <cstdarg>
#include <destoer.h>
using namespace destoer;



void load_ips_patch(const std::string &filename,std::vector<uint8_t> &rom);

std::vector<std::string> read_sorted_directory(const std::string &file_path);


template<typename... Args>
inline std::string runtime_format(const char* fmt, Args... args)
{
    return fmt::vformat(fmt,fmt::make_format_args(args...));
}