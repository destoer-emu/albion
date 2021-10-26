#pragma once
#include <destoer-emu/lib.h>
#include <destoer-emu/debug.h>
#include <gba/arm.h>
#include <gba/mem_constants.h>
#include <gba/forward_def.h>
#include <gba/mem_io.h>
#include <gba/dma.h>
#include <gba/flash.h>
namespace gameboyadvance
{


// last accessed memory region
enum class memory_region
{
    bios = 0,wram_board,wram_chip,
    io,pal,vram,oam,rom,cart_backup,
    undefined
};

// we can just switch on the top 4 bits of the addr
// thanks fleroviux
static constexpr memory_region memory_region_table[0x10] = 
{
    memory_region::bios, // 
    memory_region::undefined, // 
    memory_region::wram_board, // 
    memory_region::wram_chip,  // 
    memory_region::io, //  
    memory_region::pal, // 
    memory_region::vram, //
    memory_region::oam, //
    memory_region::rom, // waitstate 0
    memory_region::rom, // waitstate 0
    memory_region::rom, // waitstate 1
    memory_region::rom, // waitstate 1
    memory_region::rom, // waitstate 2
    memory_region::rom, // waitstate 2
    memory_region::cart_backup, // cart ram
    memory_region::cart_backup // cart ram mirror
};

// not really happy with the impl 
// so think of a better way to model it
struct Mem final
{
    Mem(GBA &gba);
    void init(std::string filename);


    template<typename access_type>
    bool fast_memcpy(u32 dst, u32 src, u32 n);

    void save_cart_ram();

    void switch_bios(bool in_bios);


    // TODO change memory accessors to
    // func pointer call when running under a debugger
    // will have to specialize the pointer for each type...

    template<typename access_type>
    u32 get_waitstates(u32 addr) const;

    template<typename access_type>
    access_type read_mem(u32 addr);
    template<typename access_type>
    access_type read_mem_handler(u32 addr);

    template<typename access_type>
    access_type read_memt(u32 addr);


    template<typename access_type>
    access_type read_memt_no_debug(u32 addr);


    // read mem
    //access handler for reads (for non io mapped mem)
    // need checks for endianess here for completeness
    template<typename access_type>
    void write_mem(u32 addr,access_type v);

    template<typename access_type>    
    void write_memt(u32 addr,access_type v);

    template<typename access_type>
    void write_memt_no_debug(u32 addr, access_type v);

    template<typename access_type>
    void tick_mem_access(u32 addr)
    {
        // only allow up to 32bit
        static_assert(sizeof(access_type) <= 4);
        cpu.cycle_tick(get_waitstates<access_type>(addr));
    }


#ifdef DEBUG
    template<typename access_type>
    using WRITE_MEM_FPTR = void (Mem::*)(u32 addr,access_type data);

    WRITE_MEM_FPTR<u32> write_u32_fptr = &Mem::write_memt_no_debug<u32>;
    WRITE_MEM_FPTR<u16> write_u16_fptr = &Mem::write_memt_no_debug<u16>;
    WRITE_MEM_FPTR<u8> write_u8_fptr  = &Mem::write_memt_no_debug<u8>;


    template<typename access_type>
    using READ_MEM_FPTR = access_type (Mem::*)(u32 addr);

    READ_MEM_FPTR<u32> read_u32_fptr = &Mem::read_memt_no_debug<u32>;
    READ_MEM_FPTR<u16> read_u16_fptr = &Mem::read_memt_no_debug<u16>;
    READ_MEM_FPTR<u8> read_u8_fptr = &Mem::read_memt_no_debug<u8>;


    void change_breakpoint_enable(bool enabled)
    {
        if(enabled)
        {
            read_u32_fptr = &Mem::read_memt<u32>;
            read_u16_fptr = &Mem::read_memt<u16>;
            read_u8_fptr = &Mem::read_memt<u8>;
            
            write_u32_fptr = &Mem::write_memt<u32>;
            write_u16_fptr = &Mem::write_memt<u16>;
            write_u8_fptr  = &Mem::write_memt<u8>;
        }

        else
        {
            read_u32_fptr = &Mem::read_memt_no_debug<u32>;
            read_u16_fptr = &Mem::read_memt_no_debug<u16>;
            read_u8_fptr = &Mem::read_memt_no_debug<u8>;
            
            write_u32_fptr = &Mem::write_memt_no_debug<u32>;
            write_u16_fptr = &Mem::write_memt_no_debug<u16>;
            write_u8_fptr  = &Mem::write_memt_no_debug<u8>;
        }
    }
#endif

    // wrapper to optimise away debug check
    void write_u8(u32 addr , u8 v)
    {
        #ifdef DEBUG
            std::invoke(write_u8_fptr,this,addr,v);
        #else
            write_memt<u8>(addr,v);
        #endif
    }

    void write_u16(u32 addr , u16 v)
    {
        #ifdef DEBUG
            std::invoke(write_u16_fptr,this,addr,v);
        #else
            write_memt_no_debug<u16>(addr,v);
        #endif
    }

    void write_u32(u32 addr, u32 v)
    {
        #ifdef DEBUG
            std::invoke(write_u32_fptr,this,addr,v);
        #else
            write_memt_no_debug<u32>(addr,v);
        #endif
    }

    u8 read_u8(u32 addr)
    {
        #ifdef DEBUG
            return std::invoke(read_u8_fptr,this,addr);
        #else
            return read_memt_no_debug<u8>(addr);
        #endif
    }

    u16 read_u16(u32 addr)
    {
        #ifdef DEBUG
            return std::invoke(read_u16_fptr,this,addr);
        #else
            return read_memt_no_debug<u16>(addr);
        #endif
    }

    u32 read_u32(u32 addr)
    {
        #ifdef DEBUG
            return std::invoke(read_u32_fptr,this,addr);
        #else
            return read_memt_no_debug<u32>(addr);
        #endif
    }

    // gba is locked to little endian
    template<typename access_type>
    access_type read_rom(u32 addr)
    {
        //return rom[addr - <whatever page start>];
        return handle_read<access_type>(rom,addr&0x1FFFFFF);        
    }



    void check_joypad_intr();

    void frame_end();

    // probablly a better way do this than to just give free reign 
    // over the array (i.e for the video stuff give display class ownership)

    // io regs
    // we split this across various regs
    // but this is how much it would take up
    //std::vector<u8> io; // 0x400 
    MemIo mem_io;

    // video ram
    std::vector<u8> vram; // 0x18000

    // display memory

    // bg/obj pallette ram
    std::vector<u8> pal_ram; // 0x400

    // object attribute map
    std::vector<u8> oam; // 0x400 

    // inited by main constructor
    Dma dma;

    u32 open_bus_value;

    // general memory
    // bios code
    std::vector<u8> bios_rom; // 0x4000

    // on board work ram
    std::vector<u8> board_wram; // 0x40000

    // on chip wram
    std::vector<u8> chip_wram; // 0x8000

    // cart save ram
    std::vector<u8> sram; // 0x8000


    struct RegionData
    {
        RegionData(u32 m, u32 s)
        {
            mask = m;
            size =s;
        }

        u32 mask;
        u32 size;
    };

    // note this assumes that accesses are in bounds
    // these masks wont work if the size is exceeded
    // i.e in the case of vram
    RegionData region_info[10]
    {
        {0x3fff,0x4000},
        {0x3ffff,0x40000},
        {0x7fff,0x8000},
        {0x3ff,0x400},
        {0x3ff,0x400},
        {0x1ffff,0x18000},
        {0x3ff,0x400},
        {(32*1024*1024)-1,32*1024*1024},
        {0,0}, // varies
        {0,0}, // not valid
    };

    u8* region_ptr[10];

    std::vector<u8*> page_table;

    Debug &debug;
    Cpu &cpu;
    Display &disp;
    Apu &apu;
    GBAScheduler &scheduler;


    // read mem helpers
    template<typename access_type>
    access_type read_bios(u32 addr);

    template<typename access_type>
    access_type read_board_wram(u32 addr);

    template<typename access_type>
    access_type read_chip_wram(u32 addr);

    template<typename access_type>
    access_type read_io(u32 addr);
    
    // underlying handler for read_io
    u8 read_io_regs(u32 addr);

    template<typename access_type>
    access_type read_pal_ram(u32 addr);

    template<typename access_type>
    access_type read_vram(u32 addr);

    template<typename access_type>
    access_type read_oam(u32 addr);

    u8 read_eeprom();

    // write mem helpers
    template<typename access_type>
    void write_board_wram(u32 addr,access_type v);

    template<typename access_type>
    void write_chip_wram(u32 addr,access_type v);

    template<typename access_type>
    void write_io(u32 addr,access_type v);

    // underlying handler for write_io
    void write_io_regs(u32 addr,u8 v);

    template<typename access_type>
    void write_pal_ram(u32 addr,access_type v);

    template<typename access_type>
    void write_vram(u32 addr,access_type v);

    template<typename access_type>
    void write_oam(u32 addr,access_type v);



    void write_eeprom(u8 v);

    bool is_eeprom(u32 addr) const;

    void update_wait_states();

    void write_timer_control(int timer,u8 v);
    u8 read_timer_counter(int timer, int idx);


    u8 *backing_vec[10] = {nullptr};
    bool can_fast_memcpy(u32 dst, u32 src,u32 n) const;
    u32 align_addr_to_region(u32 addr) const;



    enum class save_type
    {
        sram,
        eeprom,
        flash
    };

    // pick up here and impl the lookup timings
    // then go impl the cart detection
    static constexpr int cart_wait_states[3][3] = 
    {
        {5,5,5}, // sram only 8 bit
        {5,5,8}, // eeprom (dont know)
        {5,5,8} // flash only 16/32 write all reads
    };

    static constexpr size_t CART_TYPE_SIZE = 5;
    const std::array<std::string,CART_TYPE_SIZE> cart_magic = 
    {
        "EEPROM_", // 512 or 8KB
        "SRAM_", // 32kb
        "FLASH_", // 64KB
        "FLASH512_", // 64KB
        "FLASH1M_" // 128KB
    };

    static constexpr save_type save_region[CART_TYPE_SIZE] = 
    {
        save_type::eeprom,
        save_type::sram,
        save_type::flash,
        save_type::flash,
        save_type::flash
    };

    static constexpr u32 save_sizes[CART_TYPE_SIZE] = 
    {
        0x2000, // is eeprom size done by heurstic?
        0x8000,
        0x10000,
        0x10000,
        0x20000
    };



    Flash flash;

    save_type cart_type;
    size_t save_size;
    std::string filename;


    bool cart_ram_dirty = false;
    int frame_count = 0;
    static constexpr int FRAME_SAVE_LIMIT = 3600;

    // memory cycle timings
    // some can be set dynamically
    // b w h
    int wait_states[10][3];

    static constexpr int wait_states_default[10][3]  = 
    {
        {1,1,1}, // bios rom
        {1,1,1}, // wram 32k
        {3,3,6}, // wram 256k
        {1,1,1}, // io
        {1,1,2}, // pallete ram
        {1,1,2}, // vram
        {1,1,1}, // oam
        {5,5,8}, // gamepak rom (in 2nd array)
        // pretend its faster than it really is
        // until we have prefetch
        //{1,1,1}, 
        {5,5,8}, // cart backup needs to be setup depending on cart type
        // hack to undershoot timings
    };
    

    static_assert(sizeof(wait_states) == sizeof(wait_states_default));

    int rom_wait_states[3][2][3];



    enum class eeprom_state
    {
        ready,
        read_setup,
        write_setup,
        read_active
    };

    std::vector<u8> eeprom; // 0x2000
    int addr_size;
    int eeprom_idx;
    int eeprom_command;
    u32 eeprom_addr;
    uint64_t eeprom_data;
    eeprom_state state;
    u32 rom_size;

    // external memory

    // main game rom
    std::vector<u8> rom; // variable
};


// template instantsation for our memory reads
extern template u8 Mem::read_mem<u8>(u32 addr);
extern template u16 Mem::read_mem<u16>(u32 addr);
extern template u32 Mem::read_mem<u32>(u32 addr);

extern template u8 Mem::read_memt<u8>(u32 addr);
extern template u16 Mem::read_memt<u16>(u32 addr);
extern template u32 Mem::read_memt<u32>(u32 addr);

extern template u8 Mem::read_memt_no_debug<u8>(u32 addr);
extern template u16 Mem::read_memt_no_debug<u16>(u32 addr);
extern template u32 Mem::read_memt_no_debug<u32>(u32 addr);

// and for writes
extern template void Mem::write_mem<u8>(u32 addr, u8 v);
extern template void Mem::write_mem<u16>(u32 addr, u16 v);
extern template void Mem::write_mem<u32>(u32 addr, u32 v);

extern template void Mem::write_memt<u8>(u32 addr, u8 v);
extern template void Mem::write_memt<u16>(u32 addr, u16 v);
extern template void Mem::write_memt<u32>(u32 addr, u32 v);

extern template void Mem::write_memt_no_debug<u8>(u32 addr, u8 v);
extern template void Mem::write_memt_no_debug<u16>(u32 addr, u16 v);
extern template void Mem::write_memt_no_debug<u32>(u32 addr, u32 v);


extern template bool Mem::fast_memcpy<u16>(u32 src, u32 dst, u32 n);
extern template bool Mem::fast_memcpy<u32>(u32 src, u32 dst, u32 n);


extern template u32 Mem::get_waitstates<u32>(u32 addr) const;
extern template u32 Mem::get_waitstates<u16>(u32 addr) const;
extern template u32 Mem::get_waitstates<u8>(u32 addr) const;
}
