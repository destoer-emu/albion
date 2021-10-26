#pragma once
#include <gb/forward_def.h>
#include <destoer-emu/lib.h>
#include <gb/debug.h>
#include <destoer-emu/emulator.h>
#include <gb/scheduler.h>
#include <gb/rom.h>
#include <gb/mem_constants.h>

namespace gameboy
{

// function pointers have virtual checks
// even though runtime polymorphism is used nowhere if i dont add final
// nice one C++
struct Memory final
{
    Memory(GB &gb);
    void init(std::string rom_name, bool with_rom=true, bool use_bios=false);

    // disable and enable reads from the bios
    // fiddles the bank zero pointe
    bool rom_cgb_enabled() const noexcept;
    bool rom_sgb_enabled() const noexcept;
    void bios_enable() noexcept;
    void bios_disable() noexcept;


    bool is_lcd_enabled() const noexcept;

    void tick_dma(uint32_t cycles) noexcept;

    void lock_vram();
    void unlock_vram();

    using WRITE_MEM_FPTR = void (Memory::*)(uint16_t addr,uint8_t data) noexcept;
    using READ_MEM_FPTR = uint8_t (Memory::*)(uint16_t addr) const noexcept;
    using READ_MEM_MUT_FPTR = uint8_t (Memory::*)(uint16_t addr) noexcept;
#ifdef DEBUG

    WRITE_MEM_FPTR write_mem_fptr;
    READ_MEM_FPTR read_mem_fptr;

    WRITE_MEM_FPTR write_iot_fptr;
    READ_MEM_MUT_FPTR read_iot_fptr;


    void change_breakpoint_enable(bool enabled) noexcept
    {
        if(enabled)
        {
            write_mem_fptr = &Memory::write_mem_debug;
            read_mem_fptr = &Memory::read_mem_debug;
            write_iot_fptr = &Memory::write_iot_debug;
            read_iot_fptr = &Memory::read_iot_debug;               
        }

        else
        {
            write_mem_fptr = &Memory::write_mem_no_debug;
            read_mem_fptr = &Memory::read_mem_no_debug;
            write_iot_fptr = &Memory::write_iot_no_debug;
            read_iot_fptr = &Memory::read_iot_no_debug;
        }
    }


    // public access functions
    inline uint8_t read_mem(uint16_t addr) const noexcept
    {
        return std::invoke(read_mem_fptr,this,addr);
    }

    inline void write_mem(uint16_t addr, uint8_t v) noexcept
    {
        std::invoke(write_mem_fptr,this,addr,v);
    }

    uint8_t read_iot(uint16_t addr) noexcept
    {
        return std::invoke(read_iot_fptr,this,addr);
    }

    void write_iot(uint16_t addr,uint8_t v) noexcept
    {
       std::invoke(write_iot_fptr,this,addr,v);
    }


#else
    // public access functions
    inline uint8_t read_mem(uint16_t addr) const noexcept
    {
        return read_mem_no_debug(addr);
    }

    inline void write_mem(uint16_t addr, uint8_t v) noexcept
    {
        write_mem_no_debug(addr,v);
    }

    uint8_t read_iot(uint16_t addr) noexcept
    {
        return read_iot_no_debug(addr);
    }

    void write_iot(uint16_t addr,uint8_t v) noexcept
    {
        write_iot_no_debug(addr,v);
    }

#endif

    uint16_t read_word(uint16_t addr) noexcept;
    void write_word(uint16_t addr, uint16_t v) noexcept;
    uint8_t read_iot_no_debug(uint16_t addr) noexcept;

    // memory accesses (timed)
    uint8_t read_memt(uint16_t addr) noexcept;
    uint8_t read_memt_no_oam_bug(uint16_t addr) noexcept;
    void write_memt(uint16_t addr, uint8_t v) noexcept;
    void write_memt_no_oam_bug(uint16_t addr, uint8_t v) noexcept;
    uint16_t read_wordt(uint16_t addr) noexcept;
    void write_wordt(uint16_t addr, uint16_t v) noexcept;
    void write_io(uint16_t addr,uint8_t v) noexcept;
    void write_iot_no_debug(uint16_t addr,uint8_t v) noexcept;

    // public underlying memory for direct access
    // required for handling io and vram
    std::vector<uint8_t> io; // 0x100
    std::vector<std::vector<uint8_t>> vram; // 0x4000
    std::vector<uint8_t> oam; // 0xa0
    std::array<uint8_t*,16> page_table;

    // direct write access no side affects
    void raw_write(uint16_t addr, uint8_t v) noexcept;
    void raw_write_word(uint16_t addr, uint16_t v) noexcept;
    uint8_t raw_read(uint16_t addr) const noexcept;
    uint16_t raw_read_word(uint16_t addr) const noexcept;

    void update_page_table_bank();
    void update_page_table_sram();

    void frame_end();

    // save file helpers
    void save_cart_ram();
    void load_cart_ram();

    // save states
    void save_state(std::ofstream &fp);
    void load_state(std::ifstream &fp);

    void do_hdma() noexcept;


    // serial tests
    emu_test test_result = emu_test::running;

	int gekkio_pass_count = 0;
    static constexpr int GEKKIO_PASS_SIZE = 6;
	static constexpr int gekkio_pass_magic[GEKKIO_PASS_SIZE] = {3,5,8,13,21,34};

	int gekkio_fail_count = 0;
	static constexpr int GEKKIO_FAIL_SIZE = 6;

    RomInfo rom_info; 

    // oam dma 
    bool oam_dma_active = false; // indicate a dma is active and to lock memory

    Cpu &cpu;
    Ppu &ppu;
    Apu &apu;
    GameboyScheduler &scheduler;
    GBDebug &debug;

#ifdef DEBUG
    uint8_t read_mem_debug(uint16_t addr) const noexcept;
    void write_mem_debug(uint16_t addr, uint8_t v) noexcept;
    uint8_t read_iot_debug(uint16_t addr) noexcept;
    void write_iot_debug(uint16_t addr, uint8_t v) noexcept;
#endif

    uint8_t read_mem_no_debug(uint16_t addr) const noexcept;
    void write_mem_no_debug(uint16_t addr, uint8_t v) noexcept;

    void do_dma(uint8_t v) noexcept;

    void init_mem_table() noexcept;
    void init_banking_table() noexcept;

    // read mem underyling
    uint8_t read_oam(uint16_t addr) const noexcept;
    uint8_t read_vram(uint16_t addr) const noexcept;
    uint8_t read_cart_ram(uint16_t addr) const noexcept;
    uint8_t read_io(uint16_t addr) const noexcept; 
    uint8_t read_rom_bank(uint16_t addr) const noexcept;
    uint8_t read_bank_zero(uint16_t addr) const noexcept;
    uint8_t read_wram_low(uint16_t addr) const noexcept;
    uint8_t read_wram_high(uint16_t addr) const noexcept;
    uint8_t read_hram(uint16_t addr) const noexcept;

    // write mem underlying
    void write_oam(uint16_t addr,uint8_t v) noexcept;
    void write_vram(uint16_t addr,uint8_t v) noexcept;
    void write_wram_low(uint16_t addr,uint8_t v) noexcept;
    void write_wram_high(uint16_t addr,uint8_t v) noexcept;
    void write_hram(uint16_t addr,uint8_t v) noexcept;
    void write_cart_ram(uint16_t addr, uint8_t v) noexcept;

    // banking functions (when writes go to the rom area)
    void ram_bank_enable(uint16_t address, uint8_t v) noexcept;
    void banking_unused(uint16_t addr, uint8_t v) noexcept;

    // read out of the bios
    uint8_t read_bios(uint16_t addr) const noexcept;

    void oam_dma_disable() noexcept;
    void oam_dma_enable() noexcept;

    uint8_t read_oam_dma(uint16_t addr) const noexcept;

    void write_blocked(uint16_t addr, uint8_t v) noexcept;
    uint8_t read_blocked(uint16_t addr) const noexcept;



    // mbc1
    void change_lo_rom_bank_mbc1(uint16_t address, uint8_t v) noexcept;
    void mbc1_banking_change(uint16_t address, uint8_t v) noexcept; 
    void change_mode_mbc1(uint16_t address, uint8_t v) noexcept;
    void change_hi_rom_bank_mbc1() noexcept;
    void ram_bank_change_mbc1() noexcept;   
    uint8_t read_rom_lower_mbc1(uint16_t addr) const noexcept;

    // mbc3
    void change_rom_bank_mbc3(uint16_t address,uint8_t v) noexcept;
    void mbc3_ram_bank_change(uint16_t address,uint8_t v) noexcept;

    // mbc2
    void lower_bank_write_mbc2(uint16_t address, uint8_t v) noexcept;
    void write_cart_ram_mbc2(uint16_t addr, uint8_t v) noexcept;
    uint8_t read_cart_ram_mbc2(uint16_t addr) const noexcept;

    //mbc5
    void mbc5_ram_bank_change(uint16_t address,uint8_t data) noexcept;
    void change_hi_rom_bank_mbc5(uint16_t address,uint8_t data) noexcept;
    void change_lo_rom_bank_mbc5(uint16_t address,uint8_t data) noexcept;
    void ram_bank_enable_mbc5(uint16_t address, uint8_t v) noexcept;


    // cgb
    void do_gdma() noexcept;
    int hdma_len = 0; // length to transfer on a  gdma
	int hdma_len_ticked = 0; // how many total dma transfers we have done
	int dma_src = 0;
	int dma_dst = 0;
	bool hdma_active = false;


    // banking vars
    bool enable_ram = false; // is ram banking enabled
    unsigned int cart_ram_bank = 0;
	unsigned int cart_rom_bank = 1; // currently selected rom bank
	bool rom_banking = true; // is rom banking enabled
    int mbc1_bank2 = 0;

	// underlying memory
    std::vector<uint8_t> bios;
    std::vector<uint8_t> wram; // 0x1000
    std::vector<std::vector<uint8_t>> cgb_wram_bank; // 0x7000 
    std::vector<uint8_t> rom; // variable
    std::vector<std::vector<uint8_t>> cart_ram_banks;


    // sgb
    std::vector<u8> sgb_packet; // 111
    bool sgb_transfer_active;
    u32 packet_count;
    u32 packet_len;
    u32 bit_count;

    std::vector<u8> sgb_pal; // 0x1000

    bool cart_ram_dirty = false;
    int frame_count = 0;
    static constexpr int FRAME_SAVE_LIMIT = 3600;

    // oam dma
	int oam_dma_address = 0; // the source address
	int oam_dma_index = 0; // how far along the dma transfer we are    


    int cgb_wram_bank_idx = 0;  // default zero
    int vram_bank = 0; // what cgb vram bank are we in?


    bool ignore_oam_bug = false;

    struct MemoryTable
    {
        READ_MEM_FPTR read_memf = nullptr;
        WRITE_MEM_FPTR write_memf = nullptr;	
    };

    // memory access function pointers
    std::array<MemoryTable,0x10> memory_table;   
};

}