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