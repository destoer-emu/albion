#pragma once
#include <fmt/format.h>
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
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdarg.h>
#include <destoer.h>
using namespace destoer;





using ColorLut = std::array<u32,32768>;
constexpr ColorLut pop_15bpp_color_lut()
{
	ColorLut lut{};

	for(u16 c = 0; c < lut.size(); c++)
	{

		const u32 R = c & 0x1f;
		const u32 G = (c >> 5) & 0x1f;
		const u32 B = (c >> 10) & 0x1f;

		// default to standard colors until we add proper correction
		lut[c] =  B << 19 |  G << 11 | R << 3 | 0xFF000000;
	}

	return lut;
}

static constexpr ColorLut COL_15BPP_LUT = pop_15bpp_color_lut();

inline u32 convert_color(u16 color)
{
	return COL_15BPP_LUT[deset_bit(color,15)];
}

void load_ips_patch(const std::string &filename,std::vector<uint8_t> &rom);