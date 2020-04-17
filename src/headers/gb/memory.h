#pragma once
#include "forward_def.h"
#include <destoer-emu/lib.h>
#include <destoer-emu/debug.h>
#include <destoer-emu/emulator.h>
#include "rom.h"
#include "mem_constants.h"

namespace gameboy
{

class Memory
{
public:
    Memory(GB &gb);
    void init(std::string rom_name, bool with_rom=true, bool use_bios=false);

    // disable and enable reads from the bios
    // fiddles the bank zero pointe
    bool rom_cgb_enabled() const noexcept;
    void bios_enable() noexcept;
    void bios_disable() noexcept;


    bool is_lcd_enabled() const noexcept;

    void tick_dma(int cycles) noexcept;

    // public access functions
    uint8_t read_mem(uint16_t addr) noexcept;
    void write_mem(uint16_t addr, uint8_t v) noexcept;
    uint16_t read_word(uint16_t addr) noexcept;
    void write_word(uint16_t addr, uint16_t v) noexcept;
    uint8_t read_iot(uint16_t) noexcept;

    // memory accesses (timed)
    uint8_t read_memt(uint16_t addr) noexcept;
    void write_memt(uint16_t addr, uint8_t v) noexcept;
    uint16_t read_wordt(uint16_t addr) noexcept;
    void write_wordt(uint16_t addr, uint16_t v) noexcept;
    void write_iot(uint16_t addr,uint8_t v) noexcept;
    void write_io(uint16_t addr,uint8_t v) noexcept;

    // public underlying memory for direct access
    // required for handling io and vram
    std::vector<uint8_t> io; // 0x100
    std::vector<std::vector<uint8_t>> vram; // 0x4000
    std::vector<uint8_t> oam; // 0xa0

    // direct write access no side affects
    void raw_write(uint16_t addr, uint8_t v) noexcept;
    uint8_t raw_read(uint16_t addr) noexcept;

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

private:

    Cpu &cpu;
    Ppu &ppu;
    Apu &apu;
    Debug &debug;

    void do_dma(uint8_t v) noexcept;

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
    int cart_ram_bank = 0;
	int cart_rom_bank = 1; // currently selected rom bank
	bool rom_banking = true; // is rom banking enabled
    int mbc1_bank2 = 0;

	// underlying memory
    std::vector<uint8_t> bios;
    std::vector<uint8_t> wram; // 0x1000
    std::vector<std::vector<uint8_t>> cgb_wram_bank; // 0x7000 
    std::vector<uint8_t> rom; // variable
    std::vector<std::vector<uint8_t>> cart_ram_banks;


    // oam dma
	bool oam_dma_active = false; // indicate a dma is active and to lock memory
	int oam_dma_address = 0; // the source address
	int oam_dma_index = 0; // how far along the dma transfer we are    


    int cgb_wram_bank_idx = 0;  // default zero
    int vram_bank = 0; // what cgb vram bank are we in?

    using WRITE_MEM_FPTR = void (Memory::*)(uint16_t addr,uint8_t data) noexcept;
    using READ_MEM_FPTR = uint8_t (Memory::*)(uint16_t addr) const noexcept;

    typedef struct
    {
        READ_MEM_FPTR read_memf = nullptr;
        WRITE_MEM_FPTR write_memf = nullptr;	
    }Memory_table;

    // memory access function pointers
    std::array<Memory_table,0x10> memory_table;


    gameboy::Rom_info rom_info;    
};

}