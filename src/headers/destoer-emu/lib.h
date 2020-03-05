#pragma once
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
#include <fstream>
#include <type_traits>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdarg.h>
#include <fmt/format.h>


inline uint64_t current_time()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>
        (std::chrono::system_clock::now().time_since_epoch()).count();    
}

inline uint64_t time_left(uint64_t next_time)
{	
    uint64_t now = current_time();
    if(next_time <= now)
    {
        return 0;     
    }
    
    else
    {
        return next_time - now;
    }
}


inline bool ishex(char x) noexcept
{
	return (x >= 'a' && x <= 'f');
}

inline bool is_valid_hex_string(char *input_str) noexcept
{
	// disallow empty strings
	if (!*input_str) return false;

	bool is_valid = true;


	// verify our string is a valid
	for(size_t j = 0; j < strlen(input_str); j++)
	{
		// allow '0x' to specify hex
		if(j == 1 && input_str[j] == 'x')
		{
			continue;
		}

		if(!isdigit(input_str[j]) && !ishex(input_str[j]))
		{
			is_valid = false;
		}
	}
	return is_valid;
}


#define UNUSED(X) ((void)X)
void read_file(const std::string &filename, std::vector<uint8_t> &buf);


constexpr char path_seperator = std::filesystem::path::preferred_separator;
std::vector<std::string> read_sorted_directory(const std::string &file_path);
std::vector<std::string> get_dir_tree(const std::string &file_path);
std::vector<std::string> filter_ext(const std::vector<std::string> &files,const std::string &str);

inline bool is_set(uint64_t reg, int bit) noexcept
{
	return ((reg >> bit) & 1);
}

inline uint64_t set_bit(uint64_t v,int bit) noexcept
{
    return (v |= (1 << bit));
}


inline uint64_t deset_bit(uint64_t v,int bit)
{
    return (v &= ~(1 << bit));
}

inline uint8_t val_bit(uint8_t data, int position) noexcept
{
	const uint8_t mask = 1 << position;
	return ( data & mask ) ? 1 : 0;
}

// std::rotr and std::rotl in c++20 probs should be used
// for now https://stackoverflow.com/questions/776508/best-practices-for-circular-shift-rotate-operations-in-c

inline uint32_t rotl(uint32_t n, unsigned int c) noexcept
{
    const unsigned int mask = (CHAR_BIT*sizeof(n) - 1);  
    c &= mask;
    return (n<<c) | (n>>( (-c)&mask ));
}

inline uint32_t rotr(uint32_t n, unsigned int c) noexcept
{
    const unsigned int mask = (CHAR_BIT*sizeof(n) - 1);
    c &= mask;
    return (n>>c) | (n<<( (-c)&mask ));
}

 // https://stackoverflow.com/questions/5814072/sign-extend-a-nine-bit-number-in-c
inline int64_t sign_extend(int64_t x, int64_t b) noexcept
{
    /* generate the sign bit mask. 'b' is the extracted number of bits */
    int m = 1U << (b - 1);  

    /* Transform a 'b' bits unsigned number 'x' into a signed number 'r' */
    int r = (x ^ m) - m; 

    return r;
}

// is there a nicer way to do this?
inline size_t get_remaining_ifstream_size(std::ifstream &fp)
{
	auto cur = fp.tellg();
	fp.seekg(0,fp.end);
	auto sz = fp.tellg();
	fp.clear();
	fp.seekg(cur,fp.beg);
	return sz - cur;
}

// helpers for dumping data to and from binary files
template<typename T>
inline void file_write_var(std::ofstream &fp, const T &data)
{
	fp.write(reinterpret_cast<const char*>(&data),sizeof(T));
}

template<typename T>
inline void file_read_var(std::ifstream &fp, T &data)
{
	auto sz = get_remaining_ifstream_size(fp);
	if(sz < sizeof(T))
	{
		throw std::runtime_error("file_read_var error");
	}
	fp.read(reinterpret_cast<char*>(&data),sizeof(T));
}

template<typename T>
inline void file_write_arr(std::ofstream &fp, const T *data,size_t size)
{
	fp.write(reinterpret_cast<const char*>(data),size);
}

template<typename T>
inline void file_read_arr(std::ifstream &fp, T *data, size_t size)
{
	auto sz = get_remaining_ifstream_size(fp);
	if(sz < size)
	{
		throw std::runtime_error("file_read_arr error");
	}	
	fp.read(reinterpret_cast<char*>(data),size);
}

template<typename T>
inline void file_write_vec(std::ofstream &fp, const std::vector<T> &buf)
{
	fp.write(reinterpret_cast<const char*>(buf.data()),sizeof(T)*buf.size());
}

template<typename T>
inline void file_read_vec(std::ifstream &fp,std::vector<T> &buf)
{
	auto sz = get_remaining_ifstream_size(fp);
	if(sz < sizeof(T) * buf.size())
	{
		throw std::runtime_error("file_read_var error");
	}		
	fp.read(reinterpret_cast<char*>(buf.data()),sizeof(T)*buf.size());
}

// if we are compiling under msvc we cant user the overflow det builtins
// we can probably do a better impl than this...
#ifdef _MSC_VER
template <typename T,typename U, typename X>
inline bool did_overflow(T v1, U v2, X ans) noexcept
{
    return  is_set((v1 ^ ans) & (v2 ^ ans),(sizeof(T)*8)-1); 
}

template <typename T,typename U, typename X>
inline bool __builtin_add_overflow(T v1,U v2,X *ans) noexcept
{
	*ans = v1 + v2;
	return did_overflow(v1, v2, *ans);
}

template <typename T,typename U, typename X>
inline bool __builtin_sub_overflow(T v1,U v2,X *ans) noexcept
{
	*ans = v1 - v2;
	return did_overflow(v1,~v2, *ans);
}
#endif