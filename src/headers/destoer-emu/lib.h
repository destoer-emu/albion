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
#include <list>
#include <set>
#include <fstream>
#include <type_traits>
#include <atomic>
#include <optional>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdarg.h>
#include <fmt/format.h>


using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

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
	return ( (x >= 'a' && x <= 'f') || (x >= 'A' && x <= 'F') );
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
void write_file(const std::string &filename,std::vector<uint8_t> &buf);
void load_ips_patch(const std::string &filename,std::vector<uint8_t> &rom);

inline std::string remove_ext(const std::string &str)
{
	std::string ans = str;

	size_t ext_idx = ans.find_last_of("."); 
	if(ext_idx != std::string::npos)
	{
		ans = ans.substr(0, ext_idx); 	
	}
	return ans;	
}

inline std::string get_save_file_name(const std::string &filename)
{
	return remove_ext(filename) + ".sav";
}


constexpr char path_separator = std::filesystem::path::preferred_separator;
std::vector<std::string> read_sorted_directory(const std::string &file_path);
std::vector<std::string> get_dir_tree(const std::string &file_path);
std::vector<std::string> filter_ext(const std::vector<std::string> &files,const std::string &str);



template<typename T>
inline bool is_set(T reg, int bit) noexcept
{
	return ((reg >> bit) & 1);
}

template<typename T>
inline T set_bit(T v,int bit) noexcept
{
	const T shift = 1;
    return v | (shift << bit);
}

template<typename T>
inline T deset_bit(T v,int bit) noexcept
{
	const T shift = 1;
    return v & ~(shift << bit);
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

//https://stackoverflow.com/questions/42534749/signed-extension-from-24-bit-to-32-bit-in-c
template<typename T>
T sign_extend(T x, int b)
{
	T res = 1;
	res <<= b - 1;
	return (x ^ res) - res;
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
		throw std::runtime_error("file_read_vec error");
	}		
	fp.read(reinterpret_cast<char*>(buf.data()),sizeof(T)*buf.size());
}





template<typename access_type>
void handle_write(std::vector<uint8_t> &buf,uint32_t addr,access_type v)
{
	#ifdef BOUNDS_CHECK // bounds check the memory access (we are very screwed if this happens)
		if(buf.size() < addr + sizeof(access_type))
		{
			auto err = fmt::format("out of range handle write at: {:08x}:{:08x}\n",addr,buf.data());
			throw std::runtime_error(err);
		}
	#endif



	//(*(access_type*)(buf.data()+addr)) = v;
	memcpy(buf.data()+addr,&v,sizeof(access_type));
}


// need checks for endianess here for completeness
template<typename access_type>
access_type handle_read(std::vector<uint8_t> &buf,uint32_t addr)
{

	#ifdef BOUNDS_CHECK // bounds check the memory access (we are very screwed if this happens)
		if(buf.size() < addr + sizeof(access_type))
		{
			auto err = fmt::format("out of range handle read at: {:08x}:{:08x}\n",addr,buf.data());
			throw std::runtime_error(err);
		}
	#endif


	//return(*(access_type*)(buf.data()+addr));
	access_type v;
	memcpy(&v,buf.data()+addr,sizeof(access_type));  
	return v;
}


enum class token_type
{
	string,
	integer
};

struct Token
{
	Token(const std::string &l,token_type t) : literal(l), type(t)
	{

	}

	std::string literal;
	token_type type;
};

// basic tokenizer
bool tokenize(const std::string &line,std::vector<Token> &args);
uint32_t convert_imm(const std::string &imm);

void print_tokens(const std::vector<Token> &tokens);

// TODO: make this use compilier builtins where avaible
template<typename T>
inline T bswap(T x)
{
	unsigned char *buf = reinterpret_cast<unsigned char *>(&x);
	for(size_t i = 0; i < sizeof(x) / 2; i++)
	{
		std::swap(buf[i],buf[sizeof(x)-i-1]);
	}
	memcpy(&x,buf,sizeof(x));
	return x;
}

inline void unimplemented(const char *fmt, ...)
{
	printf("unimplemented: ");

	va_list args;
	va_start(args,fmt);

	vprintf(fmt,args);

	va_end(args);
	exit(1);
}


// this checks if the msb (sign) changes to something it shouldunt
// during arithmetic
template <typename T,typename U, typename X>
inline bool did_overflow(T v1, U v2, X ans) noexcept
{
    return  is_set((v1 ^ ans) & (v2 ^ ans),(sizeof(T)*8)-1); 
}



/*
thanks yaed for suggesting use of compilier builtins
*/

template<typename T>
inline bool sadd_overflow(T v1, T v2) noexcept
{
    using signed_type_in = typename std::make_signed<T>::type;
    auto sv1 = static_cast<signed_type_in>(v1);
    auto sv2 = static_cast<signed_type_in>(v2);

#ifdef _MSC_VER
	const auto ans = sv1 + sv2;
	return did_overflow(sv1, sv2, ans);  
#else
    return __builtin_add_overflow(sv1,sv2,&sv1);
#endif  
}


template<typename T>
inline bool uadd_overflow(T v1, T v2) noexcept
{
    using unsigned_type_in = typename std::make_unsigned<T>::type;
    auto uv1 = static_cast<unsigned_type_in>(v1);
    auto uv2 = static_cast<unsigned_type_in>(v2);
    
#ifdef _MSC_VER
	const auto ans = uv1 + uv2;
	return ans < uv1;
#else
    return __builtin_add_overflow(uv1,uv2,&uv1);
#endif  
}


template<typename T>
inline bool ssub_overflow(T v1,T v2) noexcept
{
    using signed_type_in = typename std::make_signed<T>::type;
    auto sv1 = static_cast<signed_type_in>(v1);
    auto sv2 = static_cast<signed_type_in>(v2);

#ifdef _MSC_VER
    const auto ans = sv1 - sv2;
    // negate 2nd operand so we can pretend
    // this is like an additon
    return did_overflow(sv1,~sv2, ans);
#else
    return __builtin_sub_overflow(sv1,sv2,&sv1);
#endif    
}


template<typename T>
inline bool usub_overflow(T v1,T v2) noexcept
{
    using unsigned_type_in = typename std::make_unsigned<T>::type;
    auto uv1 = static_cast<unsigned_type_in>(v1);
    auto uv2 = static_cast<unsigned_type_in>(v2);
#ifdef _MSC_VER
    return uv1 < uv2;
#else
	// TODO: this needs to be inverted if we compile for ARM
    return __builtin_sub_overflow(uv1,uv2,&uv1);
#endif  
}