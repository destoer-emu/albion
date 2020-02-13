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
    io.resize(0x400);
    pal_ram.resize(0x400);
    vram.resize(0x18000);
    oam.resize(0x400); 
    sram.resize(0xffff);

    std::fill(board_wram.begin(),board_wram.end(),0);
    std::fill(chip_wram.begin(),chip_wram.end(),0);
    std::fill(io.begin(),io.end(),0);
    std::fill(pal_ram.begin(),pal_ram.end(),0);
    std::fill(vram.begin(),vram.end(),0);
    std::fill(oam.begin(),oam.end(),0);
    std::fill(sram.begin(),sram.end(),0);

    
    // read out rom info here...
    std::cout << "rom size: " << rom.size() << "\n";


    // all unpressed
    io[IO_KEYINPUT] = 0xff;
    io[IO_KEYINPUT+1] = 0x3;

    ime = true;

    // read and copy in the bios rom
    read_file("GBA.BIOS",bios_rom);

    if(bios_rom.size() != 0x4000)
    {
        puts("invalid bios size!");
        exit(1);
    }
}


// this definitely needs to be cleaned up!
void Mem::write_io_regs(uint32_t addr,uint8_t v)
{
    addr &= IO_MASK;


    // unused areas (once we have the full map)
    // we can just make this be ignored in the default case!
    if(addr > 0x208 && addr < 0x300)
    {
        return;
    }


    switch(addr)
    {

        case IO_DISPCNT:
        {
            // gba / cgb mode is reserved
            io[addr] = v & ~8;
            break;
        }

        case IO_DISPCNT+1:
        {
            io[addr] = v;
            break;
        }

        // bit one toggle green swap (ingore for now)
        case IO_GREENSWAP:
        {
                
            break;
        }

        case IO_GREENSWAP+1:
        {
            break;
        }


        case IO_DISPSTAT:
        {
            // first 3 bits read only
            // 6 and 7 are unused
            io[addr] = v & ~0xc7;
            break;
        }

        
        case IO_DISPSTAT+1: // vcount (lyc)
        {
            io[addr] = v;
            break;
        }

        // read only!
        case IO_VCOUNT: // (ly)
        case IO_VCOUNT+1:
        {
            break;
        }

        // probs should store the bgcnts
        // in a easy to access array
        case IO_BG0CNT:
        case IO_BG0CNT+1:
        {
            io[addr] = v;
            break;
        }

        case IO_BG1CNT:
        case IO_BG1CNT+1:
        {
            io[addr] = v;
            break;
        }

        case IO_BG2CNT:
        case IO_BG2CNT+1:
        {
            io[addr] = v;
            break;
        }

        case IO_BG3CNT:
        case IO_BG3CNT+1:
        {
            io[addr] = v;
            break;
        }


        // all backgrounds function the same so we will group them

        case IO_BG0HOFS: // write only
        case IO_BG1HOFS:
        case IO_BG2HOFS:
        case IO_BG3HOFS:
        {
            io[addr] = v;
            break;
        }

        // only 1st bit used
        case IO_BG0HOFS+1:
        case IO_BG1HOFS+1:
        case IO_BG2HOFS+1:
        case IO_BG3HOFS+1:
        {
            io[addr] = v & 1;
            break;
        }


        case IO_BG0VOFS: // write only
        case IO_BG1VOFS:
        case IO_BG2VOFS:
        case IO_BG3VOFS:
        {
            io[addr] = v;
            break;
        }

        // only 1st bit used
        case IO_BG0VOFS+1:
        case IO_BG1VOFS+1:
        case IO_BG2VOFS+1:
        case IO_BG3VOFS+1:
        {
            io[addr] = v & 1;
            break;
        }




        // both have full write
        // bg2 scaling param X
        case IO_BG2PA:
        case IO_BG2PA+1:
        case IO_BG2PB:
        case IO_BG2PB+1:
        case IO_BG2PC:
        case IO_BG2PC+1:
        case IO_BG2PD:
        case IO_BG2PD+1:                        
        {
            io[addr] = v;
            break;
        }




        // both have full write
        // bg3 scaling param X
        case IO_BG3PA:
        case IO_BG3PA+1:
        case IO_BG3PB:
        case IO_BG3PB+1:
        case IO_BG3PC:
        case IO_BG3PC+1:
        case IO_BG3PD:
        case IO_BG3PD+1:                        
        {
            io[addr] = v;
            break;
        }




        // background 2 reference point registers
        // on write these copy to internal regs
        case IO_BG2X_L:
        case IO_BG2X_L+1:
        case IO_BG2Y_L:
        case IO_BG2Y_L+1:
        case IO_BG2X_H:
        case IO_BG2Y_H:                        
        {
            io[addr] = v;
            disp->load_reference_point_regs();
            break;
        }

        case IO_BG2X_H+1:
        case IO_BG2Y_H+1:        
        {
            io[addr] = v & ~0xf0;
            disp->load_reference_point_regs();
            break;
        }




        //rightmost cord of window plus 1
        case IO_WIN0H:
        {
            io[addr] = v;
            break;
        }

        // leftmost window cord
        case IO_WIN0H+1:
        {
            io[addr] = v;
            break;
        }


        // DMA 0

        // dma 0 source reg
        case IO_DMA0SAD: 
        case IO_DMA0SAD+1:
        case IO_DMA0SAD+2:
        case IO_DMA0SAD+3:
        {
            io[addr] = v;
            break;
        }

        // dma 0 dest reg
        case IO_DMA0DAD: 
        case IO_DMA0DAD+1:
        case IO_DMA0DAD+2:
        case IO_DMA0DAD+3:
        {
            io[addr] = v;
            break;
        }

        // dma 0 transfer len
        case IO_DMA0CNT_L:
        case IO_DMA0CNT_L+1:
        {
            io[addr] = v;
            break;
        }

        // dma 0 transfer control
        case IO_DMA0CNT_H:
        {
            io[addr] = v;
            break;
        }

        // dma 0 transfer control
        case IO_DMA0CNT_H+1:
        {
            if(is_set(v,7) && !is_set(io[addr],7)) // transfer enabeld
            {
                cpu->dma_regs[0].src = handle_read<uint32_t>(io,IO_DMA0SAD);
                cpu->dma_regs[0].dst = handle_read<uint32_t>(io,IO_DMA0DAD);
                cpu->dma_regs[0].nn = handle_read<uint16_t>(io,IO_DMA0CNT_L);       
                cpu->handle_dma(Dma_type::IMMEDIATE);
            }
            io[addr] = v;
            break;
        }







        // DMA 1

        // dma 1 source reg
        case IO_DMA1SAD: 
        case IO_DMA1SAD+1:
        case IO_DMA1SAD+2:
        case IO_DMA1SAD+3:
        {
            io[addr] = v;
            break;
        }

        // dma 1 dest reg
        case IO_DMA1DAD: 
        case IO_DMA1DAD+1:
        case IO_DMA1DAD+2:
        case IO_DMA1DAD+3:
        {
            io[addr] = v;
            break;
        }

        // dma 1 transfer len
        case IO_DMA1CNT_L:
        case IO_DMA1CNT_L+1:
        {
            io[addr] = v;
            break;
        }

        // dma 1 transfer control
        case IO_DMA1CNT_H:
        {
            io[addr] = v;
            break;
        }

        // dma 1 transfer control
        case IO_DMA1CNT_H+1:
        {
            if(is_set(v,7) && !is_set(io[addr],7)) // transfer enabeld
            {
                cpu->dma_regs[1].src = handle_read<uint32_t>(io,IO_DMA1SAD);
                cpu->dma_regs[1].dst = handle_read<uint32_t>(io,IO_DMA1DAD);
                cpu->dma_regs[1].nn = handle_read<uint16_t>(io,IO_DMA1CNT_L);   
                cpu->handle_dma(Dma_type::IMMEDIATE);
            }
            io[addr] = v;
            break;
        }



        // DMA 2

        // dma 2 source reg
        case IO_DMA2SAD: 
        case IO_DMA2SAD+1:
        case IO_DMA2SAD+2:
        case IO_DMA2SAD+3:
        {
            io[addr] = v;
            break;
        }

        // dma 2 dest reg
        case IO_DMA2DAD: 
        case IO_DMA2DAD+1:
        case IO_DMA2DAD+2:
        case IO_DMA2DAD+3:
        {
            io[addr] = v;
            break;
        }

        // dma 2 transfer len
        case IO_DMA2CNT_L:
        case IO_DMA2CNT_L+1:
        {
            io[addr] = v;
            break;
        }

        // dma 2 transfer control
        case IO_DMA2CNT_H:
        {
            io[addr] = v;
            break;
        }

        // dma 2 transfer control
        case IO_DMA2CNT_H+1:
        {
            if(is_set(v,7) && !is_set(io[addr],7)) // transfer enabeld
            {
                cpu->dma_regs[2].src = handle_read<uint32_t>(io,IO_DMA2SAD);
                cpu->dma_regs[2].dst = handle_read<uint32_t>(io,IO_DMA2DAD);
                cpu->dma_regs[2].nn = handle_read<uint16_t>(io,IO_DMA2CNT_L);   
                cpu->handle_dma(Dma_type::IMMEDIATE);
            }
            io[addr] = v;
            break;
        }



        // DMA 3

        // dma 3 source reg
        case IO_DMA3SAD: 
        case IO_DMA3SAD+1:
        case IO_DMA3SAD+2:
        case IO_DMA3SAD+3:
        {
            io[addr] = v;
            break;
        }

        // dma 3 dest reg
        case IO_DMA3DAD: 
        case IO_DMA3DAD+1:
        case IO_DMA3DAD+2:
        case IO_DMA3DAD+3:
        {
            io[addr] = v;
            break;
        }

        // dma 3 transfer len
        case IO_DMA3CNT_L:
        case IO_DMA3CNT_L+1:
        {
            io[addr] = v;
            break;
        }

        // dma 3 transfer control
        case IO_DMA3CNT_H:
        {
            io[addr] = v;
            break;
        }

        // dma 3 transfer control
        case IO_DMA3CNT_H+1:
        {
            if(is_set(v,7) && !is_set(io[addr],7)) // transfer enabeld
            {
                cpu->dma_regs[3].src = handle_read<uint32_t>(io,IO_DMA3SAD);
                cpu->dma_regs[3].dst = handle_read<uint32_t>(io,IO_DMA3DAD);
                cpu->dma_regs[3].nn = handle_read<uint16_t>(io,IO_DMA3CNT_L);                          
                cpu->handle_dma(Dma_type::IMMEDIATE);
            }
            io[addr] = v;
            break;
        }



 
        // timer 0  reload value
        case IO_TM0CNT_L:
        case IO_TM0CNT_L+1:
        {
            io[addr] = v;
            break;
        }

        // timer 0 control
        case IO_TM0CNT_H:
        {
            if(is_set(v,7) && !is_set(io[addr],7))
            {
                // reload the timer
                cpu->set_timer(0,handle_read<uint16_t>(io,IO_TM0CNT_L));
                break;
            }
            io[addr] = v;
            break;
        } 

        // unused
        case IO_TM0CNT_H+1:
        {
            break;
        }


        // timer 1  reload value
        case IO_TM1CNT_L:
        case IO_TM1CNT_L+1:
        {
            io[addr] = v;
            break;
        }

        // timer 1 control
        case IO_TM1CNT_H:
        {
            if(is_set(v,7) && !is_set(io[addr],7))
            {
                cpu->set_timer(1,handle_read<uint16_t>(io,IO_TM1CNT_L));
            }
            io[addr] = v;
            break;
        } 

        // unused
        case IO_TM1CNT_H+1:
        {
            break;
        }




        // timer 2  reload value
        case IO_TM2CNT_L:
        case IO_TM2CNT_L+1:
        {
            io[addr] = v;
            break;
        }

        // timer 2 control
        case IO_TM2CNT_H:
        {
            if(is_set(v,7) && !is_set(io[addr],7))
            {
                cpu->set_timer(2,handle_read<uint16_t>(io,IO_TM2CNT_L));
            }
            io[addr] = v;
            break;
        } 

        // unused
        case IO_TM2CNT_H+1:
        {
            break;
        }


        // timer 3  reload value
        case IO_TM3CNT_L:
        case IO_TM3CNT_L+1:
        {
            io[addr] = v;
            break;
        }

        // timer 3 control
        case IO_TM3CNT_H:
        {
            if(is_set(v,7) && !is_set(io[addr],7))
            {
                //printf("enabled! %08x\n",cpu->get_pc());
                cpu->set_timer(3,handle_read<uint16_t>(io,IO_TM3CNT_L));
            }
            io[addr] = v;
            break;
        } 

        // unused
        case IO_TM3CNT_H+1:
        {
            break;
        }



        case IO_IME: // 0th bit toggles ime
        {
            ime = is_set(v,0);
            break;
        }
        case IO_IME + 1:
        { 
            break; // do nothing
        }


        case IO_IE: // interrupt enable
        {
            io[addr] = v;
            break;
        }

        case IO_IE+1:
        {
            io[addr] = v & ~0xc0;
            break;
        }

        case IO_IF: // interrupt flag writing a 1 desets the bit
        {
            io[addr] &= ~v;
            break;
        }

        case IO_IF+1:
        {
            io[addr] &= ~(v & ~0xc0);
            break;
        }

        case IO_WAITCNT: // configures game pak access times
        {
            io[addr] = v;
            break;
        }

        case IO_WAITCNT+1: 
        {
            io[addr] = v & ~0x20;
            break;
        }        

        
        case IO_WAITCNT+2: // top half not used
        case IO_WAITCNT+3: 
        {
            break;
        }



        //unused
        case 0x400020A&IO_MASK: 
        case 0x400020B&IO_MASK:
        { 
            break;
        }

        // gba bios inits this to one to know its not in initial boot 
        case IO_POSTFLG:
        {
            io[addr] = v & 1;
            break;
        }



        default:
        {    
            //printf("unknown io reg write at %08x:%08x\n",addr,cpu->get_pc());
            io[addr] = v; // get this done fast <--- fix later
            //exit(1);
        }
    }
}


// this will require special handling
uint8_t Mem::read_io_regs(uint32_t addr)
{
    addr &= IO_MASK;
    switch(addr)
    {

        // should only be accessible from bios..
        // handle this later
        case IO_SOUNDBIAS:
        case IO_SOUNDBIAS+1:
        {
            return io[addr];
        }

        // unused
        case IO_SOUNDBIAS+2:
        case IO_SOUNDBIAS+3:
        {
            return 0; 
        }

        case IO_DISPCNT:
        {
            return io[addr];
            break;
        }

        case IO_DISPCNT+1:
        {
            return io[addr];
            break;
        }

        // these two are just stubs atm
        case IO_DISPSTAT:
        {
            return io[addr];
            break;
        }

        case IO_DISPSTAT+1:
        {
            return io[addr];
            break;
        }


        case IO_VCOUNT:
        {
            return io[addr];
            break;
        }

        case IO_VCOUNT+1: // not used
        {
            return 0x0;
            break;
        }

        // bit one toggle green swap (ingore for now)
        case IO_GREENSWAP:
        {
            return 0; 
            break;
        }

        case IO_GREENSWAP+1:
        {
            return 0;
            break;
        }






        // dma  word count
        // read only but may be read from
        // as a word to access the higher part
        case IO_DMA0CNT_L:
        case IO_DMA0CNT_L+1:
        case IO_DMA1CNT_L:
        case IO_DMA1CNT_L+1:
        case IO_DMA2CNT_L:
        case IO_DMA2CNT_L+1:                        
        case IO_DMA3CNT_L:
        case IO_DMA3CNT_L+1:
        {
            return 0;
            break;
        }


        // dma  transfer control
        case IO_DMA0CNT_H:
        case IO_DMA0CNT_H+1:
        case IO_DMA1CNT_H:
        case IO_DMA1CNT_H+1:
        case IO_DMA2CNT_H:
        case IO_DMA2CNT_H+1:                               
        case IO_DMA3CNT_H:
        case IO_DMA3CNT_H+1:
        {
            return io[addr];
            break;
        }


        // timer 0  reload value
        // return current count when read from
        case IO_TM0CNT_L:
        case IO_TM0CNT_L+1:
        {
            uint16_t timer = cpu->get_timer(0);
            return addr == IO_TM0CNT_L? timer  & 0xff : (timer >> 8) & 0xff;
            break;
        }


        // timer 1  reload value
        case IO_TM1CNT_L:
        case IO_TM1CNT_L+1:
        {
            uint16_t timer  = cpu->get_timer(1);
            return addr == IO_TM1CNT_L? timer  & 0xff : (timer >> 8) & 0xff;
            break;
        }

        // timer 2  reload value
        case IO_TM2CNT_L:
        case IO_TM2CNT_L+1:
        {
            uint16_t timer =  cpu->get_timer(2);
            return addr == IO_TM2CNT_L? timer  & 0xff : (timer >> 8) & 0xff;
            break;
        }

        // timer 3  reload value
        case IO_TM3CNT_L:
        case IO_TM3CNT_L+1:
        {
            uint16_t timer = cpu->get_timer(3);
            return addr == IO_TM3CNT_L? timer  & 0xff : (timer >> 8) & 0xff;
            break;
        }



        case IO_KEYINPUT:
        {
            return io[addr];
            break;
        }

      
        case IO_KEYINPUT+1: // 10-15 unused
        {
            return io[addr];
            break;
        }


        case IO_KEYCNT:
        {
            return io[addr];
            break;
        }

        case IO_KEYCNT+1: // 10-13 not used
        {
            return io[addr];
            break;
        }


        case IO_IME: // 0th bit toggles ime
        {
            return ime;
            break;
        }
        case IO_IME + 1:
        { 
            return 0; // do nothing
            break;
        }

        case IO_IE: // inteerupt enable
        case IO_IE+1:
        {
            return io[addr];
        }


        case IO_IF:
        case IO_IF+1:
        {
            return io[addr];
        }


        case IO_WAITCNT:
        case IO_WAITCNT+1:
        {
            return io[addr];
        }

        // unused
        case IO_WAITCNT+2:
        case IO_WAITCNT+3:
        {
            return 0;
        }


        //unused
        case 0x400020A: 
        case 0x400020B:
        { 
            return 0;
            break;
        }

        case IO_POSTFLG:
        {
            return io[addr] & 1;
            break;
        }

        default:
        {    
            //printf("unknown io reg read at %08x:%08x\n",addr,cpu->get_pc());
            //cpu->print_regs();
            //exit(1);
            return io[addr];
        }
    }
}





//access handler for reads (for non io mapped mem) 
// need checks for endianess here for completeness
template<typename access_type>
access_type Mem::handle_read(std::vector<uint8_t> &buf,uint32_t addr)
{

#ifdef DEBUG // bounds check the memory access
    if(buf.size() < addr + sizeof(access_type))
    {
        printf("out of range handle read at: %08x\n",cpu->get_pc());
        cpu->print_regs();
        exit(1);
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
    else if(addr < 0x02000000) { mem_region = UNDEFINED; return 0; }
    else if(addr < 0x03000000) v = read_board_wram<access_type>(addr);
    else if(addr < 0x04000000) v = read_chip_wram<access_type>(addr);
    else if(addr < 0x05000000) v = read_io<access_type>(addr);
    else if(addr < 0x06000000) v = read_pal_ram<access_type>(addr);
    else if(addr < 0x06018000) v = read_vram<access_type>(addr);
    else if(addr < 0x07000000) { mem_region = UNDEFINED; return 0; }
    else if(addr < 0x08000000) v = read_oam<access_type>(addr);
    else v = read_external<access_type>(addr);

    return v;
}

// unused memory is to be ignored

 // read mem unticked
template<typename access_type>
access_type Mem::read_mem(uint32_t addr)
{


#ifdef DEBUG
    uint32_t value = read_mem_handler<access_type>(addr);
    if(debug->breakpoint_hit(addr,value,break_type::read))
    {
        write_log("write breakpoint hit at {:08x}:{:08x}:{:08x}",addr,value,cpu->get_pc());
        debug->halt();
    }    
#endif

    // 28 bit bus
    addr &= 0x0fffffff;

    // handle address alignment
    addr &= ~(sizeof(access_type)-1);


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

#ifdef DEBUG
    if(debug->breakpoint_hit(addr,v,break_type::write))
    {
        write_log("write breakpoint hit at {:08x}:{:08x}:{:08x}",addr,v,cpu->get_pc());
        debug->halt();
    }   
#endif


    // 28 bit bus
    addr &= 0x0fffffff;

    // handle address alignemt
    addr &= ~(sizeof(access_type)-1);

    if(addr < 0x00004000) { mem_region = BIOS; return; } // bios is read only
    else if(addr < 0x02000000) { mem_region = UNDEFINED; return; }
    else if(addr < 0x03000000) write_board_wram<access_type>(addr,v);
    else if(addr < 0x04000000) write_chip_wram<access_type>(addr,v);
    else if(addr < 0x05000000) write_io<access_type>(addr,v);
    else if(addr < 0x06000000) write_pal_ram<access_type>(addr,v);
    else if(addr < 0x06018000) write_vram<access_type>(addr,v);
    else if(addr < 0x07000000) { mem_region = UNDEFINED; return; }
    else if(addr < 0x08000000) write_oam<access_type>(addr,v);
    else write_external<access_type>(addr,v); // rom is read only but could be flash

}

// ticked access
template<typename access_type>
void Mem::write_memt(uint32_t addr,access_type v)
{
    write_mem<access_type>(addr,v);
    tick_mem_access<access_type>();
}



template<>
void Mem::tick_mem_access<uint8_t>()
{
    // should unmapped addresses still tick a cycle?
    if(mem_region != UNDEFINED)
    {
        cpu->cycle_tick(wait_states[mem_region][BYTE]);
    }
}


template<>
void Mem::tick_mem_access<uint16_t>()
{
    // should unmapped addresses still tick a cycle?
    if(mem_region != UNDEFINED)
    {
        cpu->cycle_tick(wait_states[mem_region][HALF]);
    }
}


template<>
void Mem::tick_mem_access<uint32_t>()
{
    // should unmapped addresses still tick a cycle?
    if(mem_region != UNDEFINED)
    {
        cpu->cycle_tick(wait_states[mem_region][WORD]);
    }
}



// gba is locked to little endian
template<typename access_type>
access_type Mem::read_external(uint32_t addr)
{

    uint32_t len = rom.size();
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
            mem_region = ROM;
            //return rom[addr - 0x08000000];
            return handle_read<access_type>(rom,addr&0x1FFFFFF);
            break;
        }

        case 0xa: // wait state 1
        case 0xb:
        {
            mem_region = ROM;
            //return rom[addr - 0x0a000000];
            return handle_read<access_type>(rom,addr&0x1FFFFFF);
            break;
        }
            
        case 0xc: // wait state 2
        case 0xd:
        {
            mem_region = ROM;
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
                mem_region = SRAM;





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
                mem_region = UNDEFINED;
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
    mem_region = OAM;
    //return oam[addr & 0x3ff];
    return handle_read<access_type>(oam,addr&0x3ff);   
}

template<typename access_type>
access_type Mem::read_vram(uint32_t addr)
{
    mem_region = VRAM;
    //return vram[addr-0x06000000];
    return handle_read<access_type>(vram,addr-0x06000000);
    

}

template<typename access_type>
access_type Mem::read_pal_ram(uint32_t addr)
{
    mem_region = PAL;
    //return pal_ram[addr & 0x3ff];
    return handle_read<access_type>(pal_ram,addr&0x3ff);

}

template<typename access_type>
access_type Mem::read_board_wram(uint32_t addr)
{
    mem_region = WRAM_BOARD;
    //return board_wram[addr & 0x3ffff];
    return handle_read<access_type>(board_wram,addr&0x3ffff);
}

template<typename access_type>
access_type Mem::read_chip_wram(uint32_t addr)
{
    mem_region = WRAM_CHIP;
    //return chip_wram[addr & 0x7fff];
    return handle_read<access_type>(chip_wram,addr&0x7fff);
}

template<typename access_type>
access_type Mem::read_bios(uint32_t addr)
{
    mem_region = BIOS;
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
            mem_region = ROM;
            return;
        }

        case 0xa: // wait state 1
        case 0xb:
        {
            mem_region = ROM;
            return;
        }
            
        case 0xc: // wait state 2
        case 0xd:
        {
            mem_region = ROM;
            return;
        }

        case 0xe: // sram
        {
            if(addr <= 0x0e00ffff)
            {
                mem_region = SRAM;
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
                mem_region = UNDEFINED;
                return;
            }
        }

        case 0xf: // probably should not write here
        {
            mem_region = UNDEFINED;
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

#ifdef DEBUG // bounds check the memory access
    if(buf.size() < addr + sizeof(access_type))
    {
        printf("out of range handle write at: %08x\n",cpu->get_pc());
        cpu->print_regs();
        exit(1);
    }
#endif



    //(*(access_type*)(buf.data()+addr)) = v;
    memcpy(buf.data()+addr,&v,sizeof(access_type));
}




// as io has side effects we need to write to it byte by byte
template<>
void Mem::write_io<uint8_t>(uint32_t addr,uint8_t v)
{
    mem_region = IO;
    //io[addr & 0x3ff] = v;

    write_io_regs(addr,v);
}


template<>
void Mem::write_io<uint16_t>(uint32_t addr,uint16_t v)
{
    mem_region = IO;
    //io[addr & 0x3ff] = v;

    write_io_regs(addr,v&0x000000ff);
    write_io_regs(addr+1,(v&0x0000ff00) >> 8);
}


template<>
void Mem::write_io<uint32_t>(uint32_t addr,uint32_t v)
{
    mem_region = IO;
    //io[addr & 0x3ff] = v;

    write_io_regs(addr,v&0x000000ff);
    write_io_regs(addr+1,(v&0x0000ff00) >> 8); 
    write_io_regs(addr+2,(v&0x00ff0000) >> 16);
    write_io_regs(addr+3,(v&0xff000000) >> 24);
}



template<typename access_type>
void Mem::write_oam(uint32_t addr,access_type v)
{
    mem_region = OAM;
    //oam[addr & 0x3ff] = v;
    handle_write<access_type>(oam,addr&0x3ff,v);
}

template<typename access_type>
void Mem::write_vram(uint32_t addr,access_type v)
{
    mem_region = VRAM;
    //vram[addr-0x06000000] = v;
    handle_write<access_type>(vram,addr-0x06000000,v); 
}

template<typename access_type>
void Mem::write_pal_ram(uint32_t addr,access_type v)
{
    mem_region = PAL;
    //pal_ram[addr & 0x3ff] = v;
    handle_write<access_type>(pal_ram,addr&0x3ff,v);
}

template<typename access_type>
void Mem::write_board_wram(uint32_t addr,access_type v)
{
    mem_region = WRAM_BOARD;
    //return board_wram[addr & 0x3ffff] = v;
    handle_write<access_type>(board_wram,addr&0x3ffff,v);
}

template<typename access_type>
void Mem::write_chip_wram(uint32_t addr,access_type v)
{
    mem_region = WRAM_CHIP;
    //chip_wram[addr & 0x7fff] = v;
    handle_write<access_type>(chip_wram,addr&0x7fff,v);
}