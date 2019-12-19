#pragma once
#pragma once
#include <string>
#include <vector>
#include <array>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <functional>
#include <numeric>
#include <limits>
#include <thread>
#include <mutex>
#include <chrono>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "../fmt/format.h"


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
        return 0;
    }
    
    else
    {
        return next_time - now;
    }
}


#define UNUSED(X) ((void)X)
void read_file(std::string filename, std::vector<uint8_t> &buf);


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