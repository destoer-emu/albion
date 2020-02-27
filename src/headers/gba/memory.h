#pragma once
#include <destoer-emu/lib.h>
#include <destoer-emu/debug.h>
#include <gba/arm.h>
#include <gba/mem_constants.h>
#include <gba/forward_def.h>
#include <gba/mem_io.h>
namespace gameboyadvance
{

// not really happy with the impl 
// so think of a better way to model it
class Mem
{
public:
    void init(std::string filename,Debug *debug, Cpu *cpu, Display *disp);




    // read mem
    template<typename access_type>
    access_type handle_read(std::vector<uint8_t> &buf,uint32_t addr);

    template<typename access_type>
    access_type read_mem(uint32_t addr);
    template<typename access_type>
    access_type read_mem_handler(uint32_t addr);


    template<typename access_type>
    access_type read_memt(uint32_t addr);



    template<typename access_type>
    void handle_write(std::vector<uint8_t> &buf,uint32_t addr,access_type v);

    template<typename access_type>
    void write_mem(uint32_t addr,access_type v);

    template<typename access_type>    
    void write_memt(uint32_t addr,access_type v);

    
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


private:
    Debug *debug = nullptr;
    Cpu *cpu = nullptr;
    Display *disp = nullptr;

    template<typename access_type>
    void tick_mem_access();


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

    template<typename access_type>
    access_type read_flash(uint32_t addr);


    template<typename access_type>
    access_type read_sram(uint32_t addr);


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


    // dont know how these work
    template<typename access_type>
    void write_flash(uint32_t addr,access_type v);


    template<typename access_type>
    void write_sram(uint32_t addr,access_type v);


    // last accessed memory region
    enum class memory_region
    {
        bios = 0,wram_board,wram_chip,
        io,pal,vram,oam,rom,flash,sram,
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
        memory_region::sram, //
        memory_region::undefined
    };


    // memory cycle timings
    // some can be set dynamically
    // b w h
    int wait_states[10][3]  = 
    {
        {1,1,1}, // bios rom
        {1,1,1}, // wram 32k
        {1,1,1}, // io
        {1,1,1}, // oam
        {3,3,6}, // wram 256k
        {1,1,2}, // pallete ram
        {1,1,2}, // vram
        {5,5,8}, // gamepak rom
        {5,5,8}, // gamepak flash
        {5,5,5} // sram
    };

    memory_region mem_region;

    // general memory
    // bios code
    std::vector<uint8_t> bios_rom; // 0x4000

    // on board work ram
    std::vector<uint8_t> board_wram; // 0x40000

    // on chip wram
    std::vector<uint8_t> chip_wram; // 0x8000

    // cart save ram
    std::vector<uint8_t> sram; // 0xffff

    // external memory

    // main game rom
    std::vector<uint8_t> rom; // variable

};


// template instantsation for our memory reads
extern template uint8_t Mem::handle_read<uint8_t>(std::vector<uint8_t> &buf, uint32_t addr);
extern template uint16_t Mem::handle_read<uint16_t>(std::vector<uint8_t> &buf, uint32_t addr);
extern template uint32_t Mem::handle_read<uint32_t>(std::vector<uint8_t> &buf, uint32_t addr);

extern template uint8_t Mem::read_mem<uint8_t>(uint32_t addr);
extern template uint16_t Mem::read_mem<uint16_t>(uint32_t addr);
extern template uint32_t Mem::read_mem<uint32_t>(uint32_t addr);

extern template uint8_t Mem::read_memt<uint8_t>(uint32_t addr);
extern template uint16_t Mem::read_memt<uint16_t>(uint32_t addr);
extern template uint32_t Mem::read_memt<uint32_t>(uint32_t addr);




extern template void Mem::handle_write<uint8_t>(std::vector<uint8_t> &buf, uint32_t addr, uint8_t v);
extern template void Mem::handle_write<uint16_t>(std::vector<uint8_t> &buf, uint32_t addr, uint16_t v);
extern template void Mem::handle_write<uint32_t>(std::vector<uint8_t> &buf, uint32_t addr, uint32_t v);

extern template void Mem::write_mem<uint8_t>(uint32_t addr, uint8_t v);
extern template void Mem::write_mem<uint16_t>(uint32_t addr, uint16_t v);
extern template void Mem::write_mem<uint32_t>(uint32_t addr, uint32_t v);

extern template void Mem::write_memt<uint8_t>(uint32_t addr, uint8_t v);
extern template void Mem::write_memt<uint16_t>(uint32_t addr, uint16_t v);
extern template void Mem::write_memt<uint32_t>(uint32_t addr, uint32_t v);

}