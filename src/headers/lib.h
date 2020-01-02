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
#include "../lib/fmt/format.h"


inline uint64_t current_time()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>
        (std::chrono::system_clock::now().time_since_epoch()).count();    
}

inline uint64_t time_left(uint64_t &next_time)
{	
    uint64_t now = current_time();
    if(next_time <= now)
    {
        next_time = current_time() + 1000 / 60;
        return 0;     
    }
    
    else
    {
        return next_time - now;
    }
}


inline bool ishex(char x)
{
	return (x >= 'a' && x <= 'f');
}

inline bool is_valid_hex_string(const char *input_str)
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
void read_file(std::string filename, std::vector<uint8_t> &buf);


std::vector<std::string> read_sorted_directory(std::string file_path);

inline bool is_set(uint64_t reg, int bit)
{
	return ((reg >> bit) & 1);
}

inline uint64_t set_bit(uint64_t v,int bit)
{
    return (v |= (1 << bit));
}


inline uint64_t deset_bit(uint64_t v,int bit)
{
    return (v &= ~(1 << bit));
}

inline uint8_t val_bit(uint8_t data, int position)
{
	uint8_t mask = 1 << position;
	return ( data & mask ) ? 1 : 0;
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
