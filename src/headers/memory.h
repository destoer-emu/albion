#pragma once
#include "forward_def.h"
#include "lib.h"
#include "rom.h"
#include "mem_constants.h"

class Memory
{
public:
    void init(Cpu *c,Ppu *p,Debug *d,Apu *a,std::string rom_name, bool with_rom=true);

    bool is_lcd_enabled();

    void tick_dma(int cycles);

    // public access functions
    uint8_t read_mem(uint16_t addr);
    void write_mem(uint16_t addr, uint8_t v);
    uint16_t read_word(uint16_t addr);
    void write_word(uint16_t addr, uint16_t v);
    uint8_t read_iot(uint16_t);

    // memory accesses (timed)
    uint8_t read_memt(uint16_t addr);
    void write_memt(uint16_t addr, uint8_t v);
    uint16_t read_wordt(uint16_t addr);
    void write_wordt(uint16_t addr, uint16_t v);
    void write_iot(uint16_t addr,uint8_t v);
    void write_io(uint16_t addr,uint8_t v);

    // public underlying memory for direct access
    // required for handling io and vram
    std::vector<uint8_t> io; // 0x100
    std::vector<std::vector<uint8_t>> vram; // 0x4000
    std::vector<uint8_t> oam; // 0xa0

    // direct write access no side affects
    void raw_write(uint16_t addr, uint8_t v);
    uint8_t raw_read(uint16_t addr);

    // save file helpers
    void save_cart_ram();
    void load_cart_ram();

    // save states
    void save_state(std::ofstream &fp);
    void load_state(std::ifstream &fp);

    void do_hdma();
private:
    Cpu *cpu;
    Ppu *ppu;
    Apu *apu;
    Debug *debug;

    void do_dma(uint8_t v);

    // read mem underyling
    uint8_t read_oam(uint16_t addr);
    uint8_t read_vram(uint16_t addr);
    uint8_t read_cart_ram(uint16_t addr);
    uint8_t read_io(uint16_t addr);
    uint8_t read_rom_bank(uint16_t addr);
    uint8_t read_bank_zero(uint16_t addr);
    uint8_t read_wram_low(uint16_t addr);
    uint8_t read_wram_high(uint16_t addr);
    uint8_t read_hram(uint16_t addr);

    // write mem underlying
    void write_oam(uint16_t addr,uint8_t v);
    void write_vram(uint16_t addr,uint8_t v);
    void write_wram_low(uint16_t addr,uint8_t v);
    void write_wram_high(uint16_t addr,uint8_t v);
    void write_hram(uint16_t addr,uint8_t v);
    void write_cart_ram(uint16_t addr, uint8_t v);

    // banking functions (when writes go to the rom area)
    void ram_bank_enable(uint16_t address, uint8_t v);
    void banking_unused(uint16_t addr, uint8_t v);

    // mbc1
    void change_lo_rom_bank_mbc1(uint16_t address, uint8_t v);
    void mbc1_banking_change(uint16_t address, uint8_t v); 
    void change_mode_mbc1(uint16_t address, uint8_t v);
    void change_hi_rom_bank_mbc1(uint8_t v);
    void ram_bank_change_mbc1(uint8_t v);   
    

    // mbc3
    void change_rom_bank_mbc3(uint16_t address,uint8_t v);
    void mbc3_ram_bank_change(uint16_t address,uint8_t v);

    // mbc2
    void change_lo_rom_bank_mbc2(uint16_t address,uint8_t v);
    void ram_bank_enable_mbc2(uint16_t address,uint8_t v);

    //mbc5
    void mbc5_ram_bank_change(uint16_t address,uint8_t data);
    void change_hi_rom_bank_mbc5(uint16_t address,uint8_t data);
    void change_lo_rom_bank_mbc5(uint16_t address,uint8_t data);


    // cgb
    void do_gdma();
    int hdma_len = 0; // length to transfer on a  gdma
	int hdma_len_ticked = 0; // how many total dma transfers we have done
	int dma_src = 0;
	int dma_dst = 0;
	bool hdma_active = false;


    // banking vars
    bool enable_ram = false; // is ram banking enabled
    int cart_ram_bank = CART_RAM_BANK_INVALID;
	int cart_rom_bank = 1; // currently selected rom bank
	bool rom_banking = true; // is rom banking enabled

	// underlying memory
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

    using WRITE_MEM_FPTR = void (Memory::*)(uint16_t addr,uint8_t data);
    using READ_MEM_FPTR = uint8_t (Memory::*)(uint16_t addr);

    typedef struct
    {
        READ_MEM_FPTR read_memf = nullptr;
        WRITE_MEM_FPTR write_memf = nullptr;	
    }Memory_table;

    // memory access function pointers
    std::array<Memory_table,0x10> memory_table;


    Rom_info rom_info;    
};