#pragma once
#include <gb/forward_def.h>
#include <albion/lib.h>
#include <gb/debug.h>
#include <albion/emulator.h>
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

    void tick_dma(u32 cycles) noexcept;

    void lock_vram();
    void unlock_vram();

    using WRITE_MEM_FPTR = void (Memory::*)(u16 addr,u8 data) noexcept;
    using READ_MEM_FPTR = u8 (Memory::*)(u16 addr) const noexcept;
    using READ_MEM_MUT_FPTR = u8 (Memory::*)(u16 addr) noexcept;
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
    inline u8 read_mem(u16 addr) const noexcept
    {
        return std::invoke(read_mem_fptr,this,addr);
    }

    inline void write_mem(u16 addr, u8 v) noexcept
    {
        std::invoke(write_mem_fptr,this,addr,v);
    }

    u8 read_iot(u16 addr) noexcept
    {
        return std::invoke(read_iot_fptr,this,addr);
    }

    void write_iot(u16 addr,u8 v) noexcept
    {
       std::invoke(write_iot_fptr,this,addr,v);
    }


#else
    // public access functions
    inline u8 read_mem(u16 addr) const noexcept
    {
        return read_mem_no_debug(addr);
    }

    inline void write_mem(u16 addr, u8 v) noexcept
    {
        write_mem_no_debug(addr,v);
    }

    u8 read_iot(u16 addr) noexcept
    {
        return read_iot_no_debug(addr);
    }

    void write_iot(u16 addr,u8 v) noexcept
    {
        write_iot_no_debug(addr,v);
    }

#endif

    u16 read_word(u16 addr) noexcept;
    void write_word(u16 addr, u16 v) noexcept;
    u8 read_iot_no_debug(u16 addr) noexcept;

    // memory accesses (timed)
    u8 read_memt(u16 addr) noexcept;
    u8 read_memt_no_oam_bug(u16 addr) noexcept;
    void write_memt(u16 addr, u8 v) noexcept;
    void write_memt_no_oam_bug(u16 addr, u8 v) noexcept;
    u16 read_wordt(u16 addr) noexcept;
    void write_wordt(u16 addr, u16 v) noexcept;
    void write_io(u16 addr,u8 v) noexcept;
    void write_iot_no_debug(u16 addr,u8 v) noexcept;

    // public underlying memory for direct access
    // required for handling io and vram
    std::vector<u8> io; // 0x100
    std::vector<std::vector<u8>> vram; // 0x4000
    std::vector<u8> oam; // 0xa0
    std::array<u8*,16> page_table;

    // direct write access no side affects
    void raw_write(u16 addr, u8 v) noexcept;
    void raw_write_word(u16 addr, u16 v) noexcept;
    u8 raw_read(u16 addr) const noexcept;
    u16 raw_read_word(u16 addr) const noexcept;

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
    u8 read_mem_debug(u16 addr) const noexcept;
    void write_mem_debug(u16 addr, u8 v) noexcept;
    u8 read_iot_debug(u16 addr) noexcept;
    void write_iot_debug(u16 addr, u8 v) noexcept;
#endif

    u8 read_mem_no_debug(u16 addr) const noexcept;
    void write_mem_no_debug(u16 addr, u8 v) noexcept;

    void do_dma(u8 v) noexcept;

    void init_mem_table() noexcept;
    void init_banking_table() noexcept;

    // read mem underyling
    u8 read_oam(u16 addr) const noexcept;
    u8 read_vram(u16 addr) const noexcept;
    u8 read_cart_ram(u16 addr) const noexcept;
    u8 read_io(u16 addr) const noexcept; 
    u8 read_rom_bank(u16 addr) const noexcept;
    u8 read_bank_zero(u16 addr) const noexcept;
    u8 read_wram_low(u16 addr) const noexcept;
    u8 read_wram_high(u16 addr) const noexcept;
    u8 read_hram(u16 addr) const noexcept;

    // write mem underlying
    void write_oam(u16 addr,u8 v) noexcept;
    void write_vram(u16 addr,u8 v) noexcept;
    void write_wram_low(u16 addr,u8 v) noexcept;
    void write_wram_high(u16 addr,u8 v) noexcept;
    void write_hram(u16 addr,u8 v) noexcept;
    void write_cart_ram(u16 addr, u8 v) noexcept;

    // banking functions (when writes go to the rom area)
    void ram_bank_enable(u16 address, u8 v) noexcept;
    void banking_unused(u16 addr, u8 v) noexcept;

    // read out of the bios
    u8 read_bios(u16 addr) const noexcept;

    void oam_dma_disable() noexcept;
    void oam_dma_enable() noexcept;

    u8 read_oam_dma(u16 addr) const noexcept;

    void write_blocked(u16 addr, u8 v) noexcept;
    u8 read_blocked(u16 addr) const noexcept;



    // mbc1
    void change_lo_rom_bank_mbc1(u16 address, u8 v) noexcept;
    void mbc1_banking_change(u16 address, u8 v) noexcept; 
    void change_mode_mbc1(u16 address, u8 v) noexcept;
    void change_hi_rom_bank_mbc1() noexcept;
    void ram_bank_change_mbc1() noexcept;   
    u8 read_rom_lower_mbc1(u16 addr) const noexcept;

    // mbc3
    void change_rom_bank_mbc3(u16 address,u8 v) noexcept;
    void mbc3_ram_bank_change(u16 address,u8 v) noexcept;

    // mbc2
    void lower_bank_write_mbc2(u16 address, u8 v) noexcept;
    void write_cart_ram_mbc2(u16 addr, u8 v) noexcept;
    u8 read_cart_ram_mbc2(u16 addr) const noexcept;

    //mbc5
    void mbc5_ram_bank_change(u16 address,u8 data) noexcept;
    void change_hi_rom_bank_mbc5(u16 address,u8 data) noexcept;
    void change_lo_rom_bank_mbc5(u16 address,u8 data) noexcept;
    void ram_bank_enable_mbc5(u16 address, u8 v) noexcept;


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
    std::vector<u8> bios;
    std::vector<u8> wram; // 0x1000
    std::vector<std::vector<u8>> cgb_wram_bank; // 0x7000 
    std::vector<u8> rom; // variable
    std::vector<std::vector<u8>> cart_ram_banks;


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