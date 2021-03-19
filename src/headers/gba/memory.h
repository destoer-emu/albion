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

// not really happy with the impl 
// so think of a better way to model it
class Mem
{
public:
    Mem(GBA &gba);
    void init(std::string filename);


    template<typename access_type>
    bool fast_memcpy(uint32_t dst, uint32_t src, uint32_t n);

    void save_cart_ram();

    void switch_bios(bool in_bios);


    // TODO change memory accessors to
    // func pointer call when running under a debugger
    // will have to specialize the pointer for each type...

    template<typename access_type>
    uint32_t get_waitstates(uint32_t addr) const;


    template<typename access_type>
    access_type read_mem(uint32_t addr);
    template<typename access_type>
    access_type read_mem_handler(uint32_t addr);


    template<typename access_type>
    access_type read_memt(uint32_t addr);



    // read mem
    //access handler for reads (for non io mapped mem)
    // need checks for endianess here for completeness
    template<typename access_type>
    void write_mem(uint32_t addr,access_type v);

    template<typename access_type>    
    void write_memt(uint32_t addr,access_type v);

    void check_joypad_intr();

    void frame_end();

    // probablly a better way do this than to just give free reign 
    // over the array (i.e for the video stuff give display class ownership)

    // io regs
    // we split this across various regs
    // but this is how much it would take up
    //std::vector<uint8_t> io; // 0x400 
    MemIo mem_io;

    // video ram
    std::vector<uint8_t> vram; // 0x18000

    // display memory

    // bg/obj pallette ram
    std::vector<uint8_t> pal_ram; // 0x400

    // object attribute map
    std::vector<uint8_t> oam; // 0x400 

    // inited by main constructor
    Dma dma;
private:
    Debug &debug;
    Cpu &cpu;
    Display &disp;
    Apu &apu;
    GBAScheduler &scheduler;


    template<typename access_type>
    void tick_mem_access(uint32_t addr);


    // read mem helpers
    template<typename access_type>
    access_type read_bios(uint32_t addr);

    template<typename access_type>
    access_type read_board_wram(uint32_t addr);

    template<typename access_type>
    access_type read_chip_wram(uint32_t addr);

    template<typename access_type>
    access_type read_io(uint32_t addr);
    
    // underlying handler for read_io
    uint8_t read_io_regs(uint32_t addr);

    template<typename access_type>
    access_type read_pal_ram(uint32_t addr);

    template<typename access_type>
    access_type read_vram(uint32_t addr);

    template<typename access_type>
    access_type read_oam(uint32_t addr);

    uint8_t read_eeprom();

    template<typename access_type>
    access_type read_rom(uint32_t addr);

    // write mem helpers
    template<typename access_type>
    void write_board_wram(uint32_t addr,access_type v);

    template<typename access_type>
    void write_chip_wram(uint32_t addr,access_type v);

    template<typename access_type>
    void write_io(uint32_t addr,access_type v);

    // underlying handler for write_io
    void write_io_regs(uint32_t addr,uint8_t v);

    template<typename access_type>
    void write_pal_ram(uint32_t addr,access_type v);

    template<typename access_type>
    void write_vram(uint32_t addr,access_type v);

    template<typename access_type>
    void write_oam(uint32_t addr,access_type v);



    void write_eeprom(uint8_t v);

    bool is_eeprom(uint32_t addr) const;

    void update_wait_states();

    void write_timer_control(int timer,uint8_t v);
    uint8_t read_timer_counter(int timer, int idx);

    // last accessed memory region
    enum class memory_region
    {
        bios = 0,wram_board,wram_chip,
        io,pal,vram,oam,rom,cart_backup,
        undefined
    };

    struct RegionData
    {
        RegionData(uint32_t m, uint32_t s)
        {
            mask = m;
            size =s;
        }

        uint32_t mask;
        uint32_t size;
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


    uint8_t *backing_vec[10] = {nullptr};
    bool can_fast_memcpy(uint32_t dst, uint32_t src,uint32_t n) const;
    uint32_t align_addr_to_region(uint32_t addr) const;



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

    static constexpr uint32_t save_sizes[CART_TYPE_SIZE] = 
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


    // general memory
    // bios code
    std::vector<uint8_t> bios_rom; // 0x4000

    // on board work ram
    std::vector<uint8_t> board_wram; // 0x40000

    // on chip wram
    std::vector<uint8_t> chip_wram; // 0x8000

    // cart save ram
    std::vector<uint8_t> sram; // 0x8000

    enum class eeprom_state
    {
        ready,
        read_setup,
        write_setup,
        read_active
    };

    std::vector<uint8_t> eeprom; // 0x2000
    int addr_size;
    int eeprom_idx;
    int eeprom_command;
    uint32_t eeprom_addr;
    uint64_t eeprom_data;
    eeprom_state state;
    uint32_t rom_size;

    // external memory

    // main game rom
    std::vector<uint8_t> rom; // variable

    std::vector<uint8_t*> page_table;
};


// template instantsation for our memory reads
extern template uint8_t Mem::read_mem<uint8_t>(uint32_t addr);
extern template uint16_t Mem::read_mem<uint16_t>(uint32_t addr);
extern template uint32_t Mem::read_mem<uint32_t>(uint32_t addr);

extern template uint8_t Mem::read_memt<uint8_t>(uint32_t addr);
extern template uint16_t Mem::read_memt<uint16_t>(uint32_t addr);
extern template uint32_t Mem::read_memt<uint32_t>(uint32_t addr);

// and for writes
extern template void Mem::write_mem<uint8_t>(uint32_t addr, uint8_t v);
extern template void Mem::write_mem<uint16_t>(uint32_t addr, uint16_t v);
extern template void Mem::write_mem<uint32_t>(uint32_t addr, uint32_t v);

extern template void Mem::write_memt<uint8_t>(uint32_t addr, uint8_t v);
extern template void Mem::write_memt<uint16_t>(uint32_t addr, uint16_t v);
extern template void Mem::write_memt<uint32_t>(uint32_t addr, uint32_t v);


extern template bool Mem::fast_memcpy<uint16_t>(uint32_t src, uint32_t dst, uint32_t n);
extern template bool Mem::fast_memcpy<uint32_t>(uint32_t src, uint32_t dst, uint32_t n);


extern template uint32_t Mem::get_waitstates<uint32_t>(uint32_t addr) const;
extern template uint32_t Mem::get_waitstates<uint16_t>(uint32_t addr) const;
extern template uint32_t Mem::get_waitstates<uint8_t>(uint32_t addr) const;
}
