#include <gba/memory.h>
#include <gba/cpu.h>
#include <gba/display.h>



// template instantsation for our memory reads
template uint8_t Mem::handle_read<uint8_t>(std::vector<uint8_t> &buf, uint32_t addr);
template uint16_t Mem::handle_read<uint16_t>(std::vector<uint8_t> &buf, uint32_t addr);
template uint32_t Mem::handle_read<uint32_t>(std::vector<uint8_t> &buf, uint32_t addr);

template uint8_t Mem::read_mem<uint8_t>(uint32_t addr);
template uint16_t Mem::read_mem<uint16_t>(uint32_t addr);
template uint32_t Mem::read_mem<uint32_t>(uint32_t addr);

template uint8_t Mem::read_memt<uint8_t>(uint32_t addr);
template uint16_t Mem::read_memt<uint16_t>(uint32_t addr);
template uint32_t Mem::read_memt<uint32_t>(uint32_t addr);




template void Mem::handle_write<uint8_t>(std::vector<uint8_t> &buf, uint32_t addr, uint8_t v);
template void Mem::handle_write<uint16_t>(std::vector<uint8_t> &buf, uint32_t addr, uint16_t v);
template void Mem::handle_write<uint32_t>(std::vector<uint8_t> &buf, uint32_t addr, uint32_t v);

template void Mem::write_mem<uint8_t>(uint32_t addr, uint8_t v);
template void Mem::write_mem<uint16_t>(uint32_t addr, uint16_t v);
template void Mem::write_mem<uint32_t>(uint32_t addr, uint32_t v);

template void Mem::write_memt<uint8_t>(uint32_t addr, uint8_t v);
template void Mem::write_memt<uint16_t>(uint32_t addr, uint16_t v);
template void Mem::write_memt<uint32_t>(uint32_t addr, uint32_t v);



void Mem::init(std::string filename, Debug *debug,Cpu *cpu,Display *disp)
{
    // init component
    this->debug = debug;
    this->cpu = cpu;
    this->disp = disp;

    // read out rom
    read_file(filename,rom);

    

    // alloc our underlying system memory
    bios_rom.resize(0x4000);
    board_wram.resize(0x40000);
    chip_wram.resize(0x8000);
    pal_ram.resize(0x400);
    vram.resize(0x18000);
    oam.resize(0x400); 
    sram.resize(0xffff);

    std::fill(board_wram.begin(),board_wram.end(),0);
    std::fill(chip_wram.begin(),chip_wram.end(),0);
    std::fill(pal_ram.begin(),pal_ram.end(),0);
    std::fill(vram.begin(),vram.end(),0);
    std::fill(oam.begin(),oam.end(),0);
    std::fill(sram.begin(),sram.end(),0);

    
    // read out rom info here...
    std::cout << "rom size: " << rom.size() << "\n";

    // read and copy in the bios rom
    read_file("GBA.BIOS",bios_rom);

    if(bios_rom.size() != 0x4000)
    {
        throw std::runtime_error("invalid bios size!");
    }
}


void Mem::write_io_regs(uint32_t addr,uint8_t v)
{
    addr &= IO_MASK;

    switch(addr)
    {

        case IO_DISPCNT: disp->disp_io.disp_cnt.write(0,v); break;
        case IO_DISPCNT+1: disp->disp_io.disp_cnt.write(1,v); break;

        // stubbed
        case IO_GREENSWAP: break;
        case IO_GREENSWAP+1: break;

        case IO_BG0CNT: disp->disp_io.bg_cnt[0].write(0,v); break;
        case IO_BG0CNT+1: disp->disp_io.bg_cnt[0].write(1,v); break;

        case IO_BG1CNT: disp->disp_io.bg_cnt[1].write(0,v); break;
        case IO_BG1CNT+1: disp->disp_io.bg_cnt[1].write(1,v); break;

        case IO_BG2CNT: disp->disp_io.bg_cnt[2].write(0,v); break;
        case IO_BG2CNT+1: disp->disp_io.bg_cnt[2].write(1,v); break;

        case IO_BG3CNT: disp->disp_io.bg_cnt[3].write(0,v); break;
        case IO_BG3CNT+1: disp->disp_io.bg_cnt[3].write(1,v); break;


        case IO_BG0HOFS: disp->disp_io.bg_offset_x[0].write(0,v); break;
        case IO_BG0HOFS+1: disp->disp_io.bg_offset_x[0].write(1,v); break;
        case IO_BG0VOFS: disp->disp_io.bg_offset_y[0].write(0,v); break;
        case IO_BG0VOFS+1: disp->disp_io.bg_offset_y[0].write(1,v); break;

        case IO_BG1HOFS: disp->disp_io.bg_offset_x[1].write(0,v); break;
        case IO_BG1HOFS+1: disp->disp_io.bg_offset_x[1].write(1,v); break;
        case IO_BG1VOFS:  disp->disp_io.bg_offset_y[1].write(0,v); break;
        case IO_BG1VOFS+1: disp->disp_io.bg_offset_y[1].write(1,v); break;

        case IO_BG2HOFS: disp->disp_io.bg_offset_x[2].write(0,v); break;
        case IO_BG2HOFS+1: disp->disp_io.bg_offset_x[2].write(1,v); break;
        case IO_BG2VOFS: disp->disp_io.bg_offset_y[2].write(0,v); break;
        case IO_BG2VOFS+1: disp->disp_io.bg_offset_y[2].write(1,v); break;

        case IO_BG3HOFS: disp->disp_io.bg_offset_x[3].write(0,v); break;
        case IO_BG3HOFS+1: disp->disp_io.bg_offset_x[3].write(1,v); break;
        case IO_BG3VOFS: disp->disp_io.bg_offset_y[3].write(0,v); break;
        case IO_BG3VOFS+1: disp->disp_io.bg_offset_y[3].write(1,v); break;   

        case IO_IME: cpu->cpu_io.ime = is_set(v,0); break;
        case IO_IME+1: case IO_IME+2: case IO_IME+3: break; // stub

        default: // here we will handle open bus when we have all our io regs done :)
        { 
            auto err = fmt::format("[memory {:08x}] unhandled write at {:08x}:{:x}",cpu->get_pc(),addr,v);
            throw std::runtime_error(err);
        }
    }
}



uint8_t Mem::read_io_regs(uint32_t addr)
{
    addr &= IO_MASK;


    switch(addr)
    {
        case IO_DISPSTAT: return disp->disp_io.disp_stat.read(0);
        case IO_DISPSTAT+1: return disp->disp_io.disp_stat.read(1);

        case IO_KEYINPUT: return mem_io.keyinput;
        case IO_KEYINPUT+1: return (mem_io.keyinput >> 8) & 3;

        case IO_VCOUNT: return disp->get_vcount();
        case IO_VCOUNT+1: return disp->get_vcount();

        default:
        {
            auto err = fmt::format("[memory {:08x}] unhandled read at {:08x}",cpu->get_pc(),addr);
            throw std::runtime_error(err);
        }
    }
}





//access handler for reads (for non io mapped mem) 
// need checks for endianess here for completeness
template<typename access_type>
access_type Mem::handle_read(std::vector<uint8_t> &buf,uint32_t addr)
{

#ifdef DEBUG // bounds check the memory access (we are very screwed if this happens)
    if(buf.size() < addr + sizeof(access_type))
    {
        auto err = fmt::format("out of range handle read at: {:08x}\n",cpu->get_pc());
        throw std::runtime_error(err);
    }
#endif


    //return(*(access_type*)(buf.data()+addr));
    access_type v;
    memcpy(&v,buf.data()+addr,sizeof(access_type));  
    return v;
}

template<typename access_type>
access_type Mem::read_mem_handler(uint32_t addr)
{
    access_type v;
    if(addr < 0x00004000) v = read_bios<access_type>(addr);
    else if(addr < 0x02000000) { mem_region = memory_region::undefined; return 0; }
    else if(addr < 0x03000000) v = read_board_wram<access_type>(addr);
    else if(addr < 0x04000000) v = read_chip_wram<access_type>(addr);
    else if(addr < 0x05000000) v = read_io<access_type>(addr);
    else if(addr < 0x06000000) v = read_pal_ram<access_type>(addr);
    else if(addr < 0x06018000) v = read_vram<access_type>(addr);
    else if(addr < 0x07000000) { mem_region = memory_region::undefined; return 0; }
    else if(addr < 0x08000000) v = read_oam<access_type>(addr);
    else if(addr < 0x0E010000) v = read_external<access_type>(addr);
    else { mem_region = memory_region::undefined; return 0; }

    return v;
}

// unused memory is to be ignored

 // read mem unticked
template<typename access_type>
access_type Mem::read_mem(uint32_t addr)
{
    // only allow up to 32bit
    static_assert(sizeof(access_type) <= 4);

    // 28 bit bus
    addr &= 0x0fffffff;

    // handle address alignment
    addr &= ~(sizeof(access_type)-1);

#ifdef DEBUG
    uint32_t value = read_mem_handler<access_type>(addr);
    if(debug->breakpoint_hit(addr,value,break_type::read))
    {
        write_log("write breakpoint hit at {:08x}:{:08x}:{:08x}",addr,value,cpu->get_pc());
        debug->halt();
    }    
#endif

    access_type v = read_mem_handler<access_type>(addr);
    return v;
}

// timed memory access
template<typename access_type>
access_type Mem::read_memt(uint32_t addr)
{
    access_type v = read_mem<access_type>(addr);
    tick_mem_access<access_type>();
    return v;
}



// write mem
 // write mem unticked
template<typename access_type>
void Mem::write_mem(uint32_t addr,access_type v)
{

    // only allow up to 32bit
    static_assert(sizeof(access_type) <= 4);

    // 28 bit bus
    addr &= 0x0fffffff;

    // handle address alignemt
    addr &= ~(sizeof(access_type)-1);

#ifdef DEBUG
    if(debug->breakpoint_hit(addr,v,break_type::write))
    {
        write_log("write breakpoint hit at {:08x}:{:08x}:{:08x}",addr,v,cpu->get_pc());
        debug->halt();
    }   
#endif

    if(addr < 0x00004000) { mem_region = memory_region::bios; return; } // bios is read only
    else if(addr < 0x02000000) { mem_region = memory_region::undefined; return; }
    else if(addr < 0x03000000) write_board_wram<access_type>(addr,v);
    else if(addr < 0x04000000) write_chip_wram<access_type>(addr,v);
    else if(addr < 0x05000000) write_io<access_type>(addr,v);
    else if(addr < 0x06000000) write_pal_ram<access_type>(addr,v);
    else if(addr < 0x06018000) write_vram<access_type>(addr,v);
    else if(addr < 0x07000000) { mem_region = memory_region::undefined; return; }
    else if(addr < 0x08000000) write_oam<access_type>(addr,v);
    else if(addr < 0x0E010000) write_external<access_type>(addr,v); // rom is read only but could be flash
    else { mem_region = memory_region::undefined;  }

}

// ticked access
template<typename access_type>
void Mem::write_memt(uint32_t addr,access_type v)
{
    write_mem<access_type>(addr,v);
    tick_mem_access<access_type>();
}



template<typename access_type>
void Mem::tick_mem_access()
{
    // only allow up to 32bit
    static_assert(sizeof(access_type) <= 4);

    // should unmapped addresses still tick a cycle?
    if(mem_region != memory_region::undefined)
    {
        // access type >> 1 to get the value
        // 4 -> 2 (word)
        // 2 -> 1 (half)
        // 1 -> 0 (byte)
        cpu->cycle_tick(wait_states[static_cast<int>(mem_region)][sizeof(access_type) >> 1]);
    }
}






// gba is locked to little endian
template<typename access_type>
access_type Mem::read_external(uint32_t addr)
{

    uint32_t len = rom.size();
    // while not illegal it probably means the there are errors
    if(((addr+sizeof(access_type))&0x1FFFFFF) > len)
    {
        //printf("rom read out of range: %08x:%08x:%08x:%08x\n",addr&0x1FFFFFF,addr,len,cpu->get_pc());
        //cpu->print_regs();
        //exit(1);
        return 0;
    }

    switch((addr >> 24) & 0xf)
    {
        case 0x8: // wait state 0
        case 0x9:
        {
            mem_region = memory_region::rom;
            //return rom[addr - 0x08000000];
            return handle_read<access_type>(rom,addr&0x1FFFFFF);
            break;
        }

        case 0xa: // wait state 1
        case 0xb:
        {
            mem_region = memory_region::rom;
            //return rom[addr - 0x0a000000];
            return handle_read<access_type>(rom,addr&0x1FFFFFF);
            break;
        }
            
        case 0xc: // wait state 2
        case 0xd:
        {
            mem_region = memory_region::rom;
            //return rom[addr - 0x0c000000];
            return handle_read<access_type>(rom,addr&0x1FFFFFF);
            break;
        }

        // need to implement flash etc properly...
        case 0xe: // sram
        {
            if(addr <= 0x0e00ffff) // need save type det and handling the actual flash commands
            {
                /*
                if(mode != BYTE)
                {
                    printf("illegal sram read %08x:%08x\n",cpu->get_pc(),addr);
                    cpu->print_regs();
                    exit(1);                    
                }
                */
                mem_region = memory_region::sram;





                // ugly hack
                if((addr & 0xffff) == 1)
                {
                    return 0x09;
                }

                else if((addr & 0xffff) == 0)
                {
                    return 0xc2;
                }


                return sram[addr & 0xffff];
                //printf("sram read %08x:%08x\n",cpu->get_pc(),addr);
                //cpu->print_regs();
                //exit(1);
            }
                
            else // unused
            {
                mem_region = memory_region::undefined;
                return 0x00;
            }
        }
    }
    printf("read_external fell through %08x\n",addr);
    cpu->print_regs();
    exit(1);    
}





template<>
uint8_t Mem::read_io<uint8_t>(uint32_t addr)
{
    return read_io_regs(addr);
}

template<>
uint16_t Mem::read_io<uint16_t>(uint32_t addr)
{
    uint16_t v = read_io_regs(addr);
    v |= read_io_regs(addr+1) << 8;
    return v;
}

template<>
uint32_t Mem::read_io<uint32_t>(uint32_t addr)
{
    uint32_t v = read_io_regs(addr);
    v |= read_io_regs(addr+1) << 8;
    v |= read_io_regs(addr+2) << 16;
    v |= read_io_regs(addr+3) << 24;
    return v;
}



template<typename access_type>
access_type Mem::read_oam(uint32_t addr)
{
    mem_region = memory_region::oam;
    //return oam[addr & 0x3ff];
    return handle_read<access_type>(oam,addr&0x3ff);   
}

template<typename access_type>
access_type Mem::read_vram(uint32_t addr)
{
    mem_region = memory_region::vram;
    //return vram[addr-0x06000000];
    return handle_read<access_type>(vram,addr-0x06000000);
}

template<typename access_type>
access_type Mem::read_pal_ram(uint32_t addr)
{
    mem_region = memory_region::pal;
    //return pal_ram[addr & 0x3ff];
    return handle_read<access_type>(pal_ram,addr&0x3ff);

}

template<typename access_type>
access_type Mem::read_board_wram(uint32_t addr)
{
    mem_region = memory_region::wram_board;
    //return board_wram[addr & 0x3ffff];
    return handle_read<access_type>(board_wram,addr&0x3ffff);
}

template<typename access_type>
access_type Mem::read_chip_wram(uint32_t addr)
{
    mem_region = memory_region::wram_chip;
    //return chip_wram[addr & 0x7fff];
    return handle_read<access_type>(chip_wram,addr&0x7fff);
}

template<typename access_type>
access_type Mem::read_bios(uint32_t addr)
{
    mem_region = memory_region::bios;
    //return bios_rom[addr];
    return handle_read<access_type>(bios_rom,addr);
}


template<typename access_type>
void Mem::write_external(uint32_t addr,access_type v)
{
    // rom is read only
    switch((addr >> 24) & 0xf)
    {
        case 0x8: // wait state 0
        case 0x9:
        {
            mem_region = memory_region::rom;
            return;
        }

        case 0xa: // wait state 1
        case 0xb:
        {
            mem_region = memory_region::rom;
            return;
        }
            
        case 0xc: // wait state 2
        case 0xd:
        {
            mem_region = memory_region::rom;
            return;
        }

        case 0xe: // sram
        {
            if(addr <= 0x0e00ffff)
            {
                mem_region = memory_region::sram;
            /*    if(mode != BYTE)
                {
                    printf("invalid sram write %08x:%08x\n",cpu->get_pc(),addr);
                    cpu->print_regs();
                    exit(1);                    
                }
            */
                sram[addr & 0xfffe] = v;
                return;

                //printf("sram write %08x:%08x\n",cpu->get_pc(),addr);
                //cpu->print_regs();
                //exit(1);
            }
                
            else // unused
            {
                mem_region = memory_region::undefined;
                return;
            }
        }

        case 0xf: // probably should not write here
        {
            mem_region = memory_region::undefined;
            return;
        }

    }
    printf("write_external fell through %08x:%08x\n",addr,cpu->get_pc());
    cpu->print_regs();
    exit(1);    
}

//access handler for reads (for non io mapped mem)
// need checks for endianess here for completeness
template<typename access_type>
void Mem::handle_write(std::vector<uint8_t> &buf,uint32_t addr,access_type v)
{

#ifdef DEBUG // bounds check the memory access (we are very screwed if this happens)
    if(buf.size() < addr + sizeof(access_type))
    {
        auto err = fmt::format("out of range handle write at: {:08x}\n",cpu->get_pc());
        throw std::runtime_error(err);
    }
#endif



    //(*(access_type*)(buf.data()+addr)) = v;
    memcpy(buf.data()+addr,&v,sizeof(access_type));
}




// as io has side effects we need to write to it byte by byte
template<>
void Mem::write_io<uint8_t>(uint32_t addr,uint8_t v)
{
    mem_region = memory_region::io;
    //io[addr & 0x3ff] = v;

    write_io_regs(addr,v);
}


template<>
void Mem::write_io<uint16_t>(uint32_t addr,uint16_t v)
{
    mem_region = memory_region::io;
    //io[addr & 0x3ff] = v;

    write_io_regs(addr,v&0x000000ff);
    write_io_regs(addr+1,(v&0x0000ff00) >> 8);
}


template<>
void Mem::write_io<uint32_t>(uint32_t addr,uint32_t v)
{
    mem_region = memory_region::io;
    //io[addr & 0x3ff] = v;

    write_io_regs(addr,v&0x000000ff);
    write_io_regs(addr+1,(v&0x0000ff00) >> 8); 
    write_io_regs(addr+2,(v&0x00ff0000) >> 16);
    write_io_regs(addr+3,(v&0xff000000) >> 24);
}



template<typename access_type>
void Mem::write_oam(uint32_t addr,access_type v)
{
    mem_region = memory_region::oam;
    //oam[addr & 0x3ff] = v;
    handle_write<access_type>(oam,addr&0x3ff,v);
}

template<typename access_type>
void Mem::write_vram(uint32_t addr,access_type v)
{
    mem_region = memory_region::vram;
    //vram[addr-0x06000000] = v;
    handle_write<access_type>(vram,addr-0x06000000,v); 
}

template<typename access_type>
void Mem::write_pal_ram(uint32_t addr,access_type v)
{
    mem_region = memory_region::pal;
    //pal_ram[addr & 0x3ff] = v;
    handle_write<access_type>(pal_ram,addr&0x3ff,v);
}

template<typename access_type>
void Mem::write_board_wram(uint32_t addr,access_type v)
{
    mem_region = memory_region::wram_board;
    //return board_wram[addr & 0x3ffff] = v;
    handle_write<access_type>(board_wram,addr&0x3ffff,v);
}

template<typename access_type>
void Mem::write_chip_wram(uint32_t addr,access_type v)
{
    mem_region = memory_region::wram_chip;
    //chip_wram[addr & 0x7fff] = v;
    handle_write<access_type>(chip_wram,addr&0x7fff,v);
}