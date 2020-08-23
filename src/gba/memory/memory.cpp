#include <gba/gba.h>

namespace gameboyadvance
{

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


Mem::Mem(GBA &gba) : dma{gba}, debug(gba.debug), cpu(gba.cpu), 
    disp(gba.disp), apu(gba.apu)
{
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
}

void Mem::init(std::string filename)
{
    // read out rom
    read_file(filename,rom);

    

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
    mem_io.init();
    dma.init();
}


void Mem::write_io_regs(uint32_t addr,uint8_t v)
{
    addr &= IO_MASK;

    // io not mirrored bar one undocumented register
    if(addr >= 0x400)
    {
        return;
    }


    // add to unused handler in default later
    // we just want this range of unused addrs
    // to be totally ignore for now for convenice
    if(addr >= 0x20a && addr <= 0x2ff)
    {
        return;
    }

    switch(addr)
    {

        case IO_DISPCNT: disp.disp_io.disp_cnt.write(0,v); break;
        case IO_DISPCNT+1: disp.disp_io.disp_cnt.write(1,v); break;

        // read only remove later!
        case IO_VCOUNT: break;
        case IO_VCOUNT+1: break;

        case IO_DISPSTAT: disp.disp_io.disp_stat.write(0,v); break;
        case IO_DISPSTAT+1:
        {
            disp.disp_io.disp_stat.write(1,v);
            // new lyc written need to re run the comparison 
            disp.update_vcount_compare();
            break;
        }

        // stubbed
        case IO_GREENSWAP: break;
        case IO_GREENSWAP+1: break;

        case IO_BG0CNT: disp.disp_io.bg_cnt[0].write(0,v); break;
        case IO_BG0CNT+1: disp.disp_io.bg_cnt[0].write(1,v); break;

        case IO_BG1CNT: disp.disp_io.bg_cnt[1].write(0,v); break;
        case IO_BG1CNT+1: disp.disp_io.bg_cnt[1].write(1,v); break;

        case IO_BG2CNT: disp.disp_io.bg_cnt[2].write(0,v); break;
        case IO_BG2CNT+1: disp.disp_io.bg_cnt[2].write(1,v); break;

        case IO_BG3CNT: disp.disp_io.bg_cnt[3].write(0,v); break;
        case IO_BG3CNT+1: disp.disp_io.bg_cnt[3].write(1,v); break;


        // bg 2 scalaing / rotation params
        case IO_BG2PA: disp.disp_io.bg2pa.write(0,v); break;
        case IO_BG2PA+1: disp.disp_io.bg2pa.write(1,v); break;

        case IO_BG2PB: disp.disp_io.bg2pb.write(0,v); break;
        case IO_BG2PB+1: disp.disp_io.bg2pb.write(1,v); break;

        case IO_BG2PC: disp.disp_io.bg2pc.write(0,v); break;
        case IO_BG2PC+1: disp.disp_io.bg2pc.write(1,v); break;

        case IO_BG2PD: disp.disp_io.bg2pd.write(0,v); break;
        case IO_BG2PD+1: disp.disp_io.bg2pd.write(1,v); break;

        // bg2 reference point
        case IO_BG2X_L: disp.disp_io.bg2x.write(0,v); break;
        case IO_BG2X_L+1: disp.disp_io.bg2x.write(1,v); break;
        case IO_BG2X_H: disp.disp_io.bg2x.write(2,v); break;
        case IO_BG2X_H+1: disp.disp_io.bg2x.write(3,v); break;

        case IO_BG2Y_L: disp.disp_io.bg2y.write(0,v); break;
        case IO_BG2Y_L+1: disp.disp_io.bg2y.write(1,v); break;
        case IO_BG2Y_H: disp.disp_io.bg2y.write(2,v); break;
        case IO_BG2Y_H+1: disp.disp_io.bg2y.write(3,v); break;


        // bg 3 scalaing / rotation params
        case IO_BG3PA: disp.disp_io.bg3pa.write(0,v); break;
        case IO_BG3PA+1: disp.disp_io.bg3pa.write(1,v); break;

        case IO_BG3PB: disp.disp_io.bg3pb.write(0,v); break;
        case IO_BG3PB+1: disp.disp_io.bg3pb.write(1,v); break;

        case IO_BG3PC: disp.disp_io.bg3pc.write(0,v); break;
        case IO_BG3PC+1: disp.disp_io.bg3pc.write(1,v); break;

        case IO_BG3PD: disp.disp_io.bg3pd.write(0,v); break;
        case IO_BG3PD+1: disp.disp_io.bg3pd.write(1,v); break;


        // bg3 reference point
        case IO_BG3X_L: disp.disp_io.bg3x.write(0,v); break;
        case IO_BG3X_L+1: disp.disp_io.bg3x.write(1,v); break;
        case IO_BG3X_H: disp.disp_io.bg3x.write(2,v); break;
        case IO_BG3X_H+1: disp.disp_io.bg3x.write(3,v); break;

        case IO_BG3Y_L: disp.disp_io.bg3y.write(0,v); break;
        case IO_BG3Y_L+1: disp.disp_io.bg3y.write(1,v); break;
        case IO_BG3Y_H: disp.disp_io.bg3y.write(2,v); break;
        case IO_BG3Y_H+1: disp.disp_io.bg3y.write(3,v); break;


        case IO_WIN0H: disp.disp_io.win0h.write(0,v); break;
        case IO_WIN0H+1: disp.disp_io.win0h.write(1,v); break;

        case IO_WIN1H: disp.disp_io.win1h.write(0,v); break;
        case IO_WIN1H+1: disp.disp_io.win1h.write(1,v); break;

        case IO_WIN0V: disp.disp_io.win0v.write(0,v); break;
        case IO_WIN0V+1: disp.disp_io.win0v.write(1,v); break;

        case IO_WIN1V: disp.disp_io.win1v.write(0,v); break;
        case IO_WIN1V+1: disp.disp_io.win1v.write(1,v); break;

        case IO_WININ: disp.disp_io.win_in.write(0,v); break;
        case IO_WININ+1: disp.disp_io.win_in.write(1,v); break;

        case IO_WINOUT: disp.disp_io.win_out.write(0,v); break;
        case IO_WINOUT+1: disp.disp_io.win_out.write(1,v); break;


        case IO_BG0HOFS: disp.disp_io.bg_offset_x[0].write(0,v); break;
        case IO_BG0HOFS+1: disp.disp_io.bg_offset_x[0].write(1,v); break;
        case IO_BG0VOFS: disp.disp_io.bg_offset_y[0].write(0,v); break;
        case IO_BG0VOFS+1: disp.disp_io.bg_offset_y[0].write(1,v); break;

        case IO_BG1HOFS: disp.disp_io.bg_offset_x[1].write(0,v); break;
        case IO_BG1HOFS+1: disp.disp_io.bg_offset_x[1].write(1,v); break;
        case IO_BG1VOFS:  disp.disp_io.bg_offset_y[1].write(0,v); break;
        case IO_BG1VOFS+1: disp.disp_io.bg_offset_y[1].write(1,v); break;

        case IO_BG2HOFS: disp.disp_io.bg_offset_x[2].write(0,v); break;
        case IO_BG2HOFS+1: disp.disp_io.bg_offset_x[2].write(1,v); break;
        case IO_BG2VOFS: disp.disp_io.bg_offset_y[2].write(0,v); break;
        case IO_BG2VOFS+1: disp.disp_io.bg_offset_y[2].write(1,v); break;

        case IO_BG3HOFS: disp.disp_io.bg_offset_x[3].write(0,v); break;
        case IO_BG3HOFS+1: disp.disp_io.bg_offset_x[3].write(1,v); break;
        case IO_BG3VOFS: disp.disp_io.bg_offset_y[3].write(0,v); break;
        case IO_BG3VOFS+1: disp.disp_io.bg_offset_y[3].write(1,v); break;   



        // timers

        case IO_TM0CNT_L: cpu.cpu_io.timers[0].write_counter(0,v); break;
        case IO_TM0CNT_L+1: cpu.cpu_io.timers[0].write_counter(1,v); break;
        case IO_TM0CNT_H: cpu.cpu_io.timers[0].write_control(v); break;
        case IO_TM0CNT_H+1: break; // upper byte not used

        case IO_TM1CNT_L: cpu.cpu_io.timers[1].write_counter(0,v); break;
        case IO_TM1CNT_L+1: cpu.cpu_io.timers[1].write_counter(1,v); break;
        case IO_TM1CNT_H: cpu.cpu_io.timers[1].write_control(v); break;
        case IO_TM1CNT_H+1: break; // upper byte not used

        case IO_TM2CNT_L: cpu.cpu_io.timers[2].write_counter(0,v); break;
        case IO_TM2CNT_L+1: cpu.cpu_io.timers[2].write_counter(1,v); break;
        case IO_TM2CNT_H: cpu.cpu_io.timers[2].write_control(v); break;
        case IO_TM2CNT_H+1: break; // upper byte not used

        case IO_TM3CNT_L: cpu.cpu_io.timers[3].write_counter(0,v); break;
        case IO_TM3CNT_L+1: cpu.cpu_io.timers[3].write_counter(1,v); break;
        case IO_TM3CNT_H: cpu.cpu_io.timers[3].write_control(v); break;
        case IO_TM3CNT_H+1: break; // upper byte not used


        // dma 0
        case IO_DMA0SAD: dma.write_source(0,0,v); break;
        case IO_DMA0SAD+1: dma.write_source(0,1,v); break;
        case IO_DMA0SAD+2: dma.write_source(0,2,v); break;
        case IO_DMA0SAD+3: dma.write_source(0,3,v); break;

        case IO_DMA0DAD:  dma.write_dest(0,0,v); break;
        case IO_DMA0DAD+1: dma.write_dest(0,1,v); break;
        case IO_DMA0DAD+2: dma.write_dest(0,2,v); break;
        case IO_DMA0DAD+3: dma.write_dest(0,3,v); break;

        case IO_DMA0CNT_L: dma.write_count(0,0,v); break;
        case IO_DMA0CNT_L+1: dma.write_count(0,1,v); break;

        case IO_DMA0CNT_H:  dma.write_control(0,0,v); break;
        case IO_DMA0CNT_H+1: dma.write_control(0,1,v); break;



        // dma 1
        case IO_DMA1SAD: dma.write_source(1,0,v); break;
        case IO_DMA1SAD+1: dma.write_source(1,1,v); break;
        case IO_DMA1SAD+2: dma.write_source(1,2,v); break;
        case IO_DMA1SAD+3: dma.write_source(1,3,v); break;

        case IO_DMA1DAD:  dma.write_dest(1,0,v); break;
        case IO_DMA1DAD+1: dma.write_dest(1,1,v); break;
        case IO_DMA1DAD+2: dma.write_dest(1,2,v); break;
        case IO_DMA1DAD+3: dma.write_dest(1,3,v); break;

        case IO_DMA1CNT_L: dma.write_count(1,0,v); break;
        case IO_DMA1CNT_L+1: dma.write_count(1,1,v); break;

        case IO_DMA1CNT_H:  dma.write_control(1,0,v); break;
        case IO_DMA1CNT_H+1: dma.write_control(1,1,v); break;



        // dma 2
        case IO_DMA2SAD: dma.write_source(2,0,v); break;
        case IO_DMA2SAD+1: dma.write_source(2,1,v); break;
        case IO_DMA2SAD+2: dma.write_source(2,2,v); break;
        case IO_DMA2SAD+3: dma.write_source(2,3,v); break;

        case IO_DMA2DAD:  dma.write_dest(2,0,v); break;
        case IO_DMA2DAD+1: dma.write_dest(2,1,v); break;
        case IO_DMA2DAD+2: dma.write_dest(2,2,v); break;
        case IO_DMA2DAD+3: dma.write_dest(2,3,v); break;

        case IO_DMA2CNT_L: dma.write_count(2,0,v); break;
        case IO_DMA2CNT_L+1: dma.write_count(2,1,v); break;

        case IO_DMA2CNT_H:  dma.write_control(2,0,v); break;
        case IO_DMA2CNT_H+1: dma.write_control(2,1,v); break;


        // dma 3
        case IO_DMA3SAD: dma.write_source(3,0,v); break;
        case IO_DMA3SAD+1: dma.write_source(3,1,v); break;
        case IO_DMA3SAD+2: dma.write_source(3,2,v); break;
        case IO_DMA3SAD+3: dma.write_source(3,3,v); break;

        case IO_DMA3DAD:  dma.write_dest(3,0,v); break;
        case IO_DMA3DAD+1: dma.write_dest(3,1,v); break;
        case IO_DMA3DAD+2: dma.write_dest(3,2,v); break;
        case IO_DMA3DAD+3: dma.write_dest(3,3,v); break;

        case IO_DMA3CNT_L: dma.write_count(3,0,v); break;
        case IO_DMA3CNT_L+1: dma.write_count(3,1,v); break;

        case IO_DMA3CNT_H:  dma.write_control(3,0,v); break;
        case IO_DMA3CNT_H+1: dma.write_control(3,1,v); break;


        // stubbed
        case IO_SOUNDCNT_H: apu.apu_io.sound_cnt.write_h(0,v); break;
        case IO_SOUNDCNT_H+1: apu.apu_io.sound_cnt.write_h(1,v); break;

        // stubbed
        case IO_SOUNDCNT_X: break;
        case IO_SOUNDCNT_X+1: break; // unused
        case IO_SOUNDCNT_X+2: break; // unused
        case IO_SOUNDCNT_X+3: break; // unused

        // fifo a
        case IO_FIFO_A: apu.apu_io.fifo_a.write(static_cast<int8_t>(v)); break;
        case IO_FIFO_A+1: apu.apu_io.fifo_a.write(static_cast<int8_t>(v)); break;
        case IO_FIFO_A+2: apu.apu_io.fifo_a.write(static_cast<int8_t>(v)); break;
        case IO_FIFO_A+3: apu.apu_io.fifo_a.write(static_cast<int8_t>(v)); break;

        // fifo b
        case IO_FIFO_B: apu.apu_io.fifo_b.write(static_cast<int8_t>(v)); break;
        case IO_FIFO_B+1: apu.apu_io.fifo_b.write(static_cast<int8_t>(v)); break;
        case IO_FIFO_B+2: apu.apu_io.fifo_b.write(static_cast<int8_t>(v)); break;
        case IO_FIFO_B+3: apu.apu_io.fifo_b.write(static_cast<int8_t>(v)); break;

        case IO_KEYCNT: 
        {
            mem_io.key_control.write(0,v);
            check_joypad_intr(); 
            break;
        }

        case IO_KEYCNT+1: 
        {
            mem_io.key_control.write(1,v); 
            check_joypad_intr();
            break;
        }

        case IO_IME: cpu.cpu_io.ime = is_set(v,0); break;
        case IO_IME+1: case IO_IME+2: case IO_IME+3: break; // unused

        case IO_IE: cpu.cpu_io.interrupt_enable = (cpu.cpu_io.interrupt_enable & 0xff00) | v; break;
        case IO_IE+1: cpu.cpu_io.interrupt_enable = (cpu.cpu_io.interrupt_enable & 0x00ff) | ((v & 0x3f) << 8);  break;

        case IO_IF: cpu.cpu_io.interrupt_flag = (cpu.cpu_io.interrupt_flag & 0xff00) & ~v; break;
        case IO_IF+1: cpu.cpu_io.interrupt_flag = (cpu.cpu_io.interrupt_flag & 0x00ff) & ~((v & 0x3f) << 8);  break;


        case IO_HALTCNT: cpu.cpu_io.halt_cnt.write(v); break;

        // gamepak wait timings ignore for now
        case IO_WAITCNT: break;
        case IO_WAITCNT+1: break;
        case IO_WAITCNT+2: break;
        case IO_WAITCNT+3: break;

        default: // here we will handle open bus when we have all our io regs done :)
        { 
            //auto err = fmt::format("[io {:08x}] unhandled write at {:08x}:{:x}",cpu.get_pc(),addr,v);
            //throw std::runtime_error(err);
        }
    }
}



uint8_t Mem::read_io_regs(uint32_t addr)
{
    addr &= IO_MASK;

    // io not mirrored bar one undocumented register
    if(addr >= 0x400)
    {
        // not sure if open bus should occur here
        return 0;
    }


    switch(addr)
    {
        
        case IO_DISPCNT: return disp.disp_io.disp_cnt.read(0);
        case IO_DISPCNT+1: return disp.disp_io.disp_cnt.read(1); 

        // stubbed for now
        case IO_GREENSWAP: return 0;
        case IO_GREENSWAP+1: return 0;


        case IO_DISPSTAT: return disp.disp_io.disp_stat.read(0);
        case IO_DISPSTAT+1: return disp.disp_io.disp_stat.read(1);

        case IO_KEYINPUT: return mem_io.keyinput;
        case IO_KEYINPUT+1: return (mem_io.keyinput >> 8) & 3;

        case IO_KEYCNT: return mem_io.key_control.read(0);
        case IO_KEYCNT+1: return mem_io.key_control.read(1);


        case IO_VCOUNT: return disp.get_vcount();
        case IO_VCOUNT+1: return 0;

        case IO_WININ: return disp.disp_io.win_in.read(0);
        case IO_WININ+1: return disp.disp_io.win_in.read(1);

        case IO_WINOUT: return disp.disp_io.win_out.read(0);
        case IO_WINOUT+1: return disp.disp_io.win_out.read(1); 


        // timers

        case IO_TM0CNT_L: return cpu.cpu_io.timers[0].read_counter(0);
        case IO_TM0CNT_L+1: return cpu.cpu_io.timers[0].read_counter(1);
        case IO_TM0CNT_H: return cpu.cpu_io.timers[0].read_control(); 
        case IO_TM0CNT_H+1: return 0; break; // upper byte not used

        case IO_TM1CNT_L: return cpu.cpu_io.timers[1].read_counter(0); 
        case IO_TM1CNT_L+1: return cpu.cpu_io.timers[1].read_counter(1); 
        case IO_TM1CNT_H: return cpu.cpu_io.timers[1].read_control();
        case IO_TM1CNT_H+1: return 0; // upper byte not used

        case IO_TM2CNT_L: return cpu.cpu_io.timers[2].read_counter(0); 
        case IO_TM2CNT_L+1: return cpu.cpu_io.timers[2].read_counter(1); 
        case IO_TM2CNT_H: return cpu.cpu_io.timers[2].read_control(); 
        case IO_TM2CNT_H+1: return 0; // upper byte not used

        case IO_TM3CNT_L: return cpu.cpu_io.timers[3].read_counter(0); 
        case IO_TM3CNT_L+1: return cpu.cpu_io.timers[3].read_counter(1);
        case IO_TM3CNT_H: return cpu.cpu_io.timers[3].read_control(); 
        case IO_TM3CNT_H+1: return 0; // upper byte not used


        case IO_IME: return cpu.cpu_io.ime;
        case IO_IME+1: case IO_IME+2: case IO_IME+3: return 0; // stub

        case IO_IE: return cpu.cpu_io.interrupt_enable & 0xff;
        case IO_IE+1: return (cpu.cpu_io.interrupt_enable >> 8) & 0x3f; 

        case IO_IF: return cpu.cpu_io.interrupt_flag & 0xff;
        case IO_IF+1: return (cpu.cpu_io.interrupt_flag >> 8) & 0x3f;         

        // gamepak wait timings ignore for now
        case IO_WAITCNT: return 0; break;
        case IO_WAITCNT+1: return 0; break;
        case IO_WAITCNT+2: return  0; break;
        case IO_WAITCNT+3: return 0; break;

        default:
        {
            //auto err = fmt::format("[io {:08x}] unhandled read at {:08x}",cpu.get_pc(),addr);
            //throw std::runtime_error(err);
            return 0;
        }
    }
}

// todo write a proper test rom to verify this
// as the quirks of how its triggered aernt docced in gbatek
void Mem::check_joypad_intr()
{
    const auto &key_control = mem_io.key_control;
    const auto &keyinput = mem_io.keyinput;


   

    if(key_control.irq_enable_flag)
    {
        bool fire = false;

        // need to intvert key input as low means its pressed

        // one pressed
        if(!key_control.irq_cond)
        {
            const int res = (key_control.key_cnt & 0x3ff) & (~keyinput & 0x3FF); 

            // if any key is pressed we care about fire
            fire = res > 0;
        }

        else
        {
            // all pressed
            fire = (~keyinput & 0x3FF) == (key_control.key_cnt & 0x3ff);
        }

        if(fire)
        {
            cpu.request_interrupt(interrupt::keypad);    
        }
    }
}


//access handler for reads (for non io mapped mem) 
// need checks for endianess here for completeness
template<typename access_type>
access_type Mem::handle_read(std::vector<uint8_t> &buf,uint32_t addr)
{

#ifdef BOUNDS_CHECK // bounds check the memory access (we are very screwed if this happens)
    if(buf.size() < addr + sizeof(access_type))
    {
        auto err = fmt::format("out of range handle read at: {:08x}:{:08x}\n",
            cpu.get_pc(),addr);
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

    mem_region = memory_region_table[(addr >> 24) & 0xf];

    switch(mem_region)
    {
        case memory_region::bios: return read_bios<access_type>(addr);
        case memory_region::wram_board: return read_board_wram<access_type>(addr);
        case memory_region::wram_chip: return read_chip_wram<access_type>(addr);
        case memory_region::io: return read_io<access_type>(addr);
        case memory_region::pal: return read_pal_ram<access_type>(addr);
        case memory_region::vram: return read_vram<access_type>(addr);
        case memory_region::oam: return read_oam<access_type>(addr);
        case memory_region::rom: return read_rom<access_type>(addr);

        // flash is also accesed here
        // we should really switch over to fptrs so this is nicer to swap stuff out
        case memory_region::sram:
        { 

            // flash id stub for pokemon remove later
            if(addr == 0x0E000000)
            {
                return 0xc2;
            }

            else if(addr == 0x0E000001)
            {
                return 0x09;
            }


            return read_sram<access_type>(addr);
        }

        default: return 0; // handle undefined accesses here
    }
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
    if(debug.breakpoint_hit(addr,value,break_type::read))
    {
        write_log(debug,"write breakpoint hit at {:08x}:{:08x}:{:08x}",addr,value,cpu.get_pc());
        debug.halt();
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
    if(debug.breakpoint_hit(addr,v,break_type::write))
    {
        write_log(debug,"write breakpoint hit at {:08x}:{:08x}:{:08x}",addr,v,cpu.get_pc());
        debug.halt();
    }   
#endif

    mem_region = memory_region_table[(addr >> 24) & 0xf];

    switch(mem_region)
    {
        case memory_region::bios: break; // read only
        case memory_region::wram_board: write_board_wram<access_type>(addr,v); break;
        case memory_region::wram_chip: write_chip_wram<access_type>(addr,v); break;
        case memory_region::io: write_io<access_type>(addr,v); break;
        case memory_region::pal: write_pal_ram<access_type>(addr,v); break;
        case memory_region::vram: write_vram<access_type>(addr,v); break;
        case memory_region::oam: write_oam<access_type>(addr,v); break;
        case memory_region::rom: break;

        // flash is also accessed here 
        // we will have to set mem_region by hand when it is
        case memory_region::sram:
        { 
            write_sram<access_type>(addr,v); 
            break;
        }

        default: break;
    }

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
        // our waitstates are busted ive stubbed them for now
        // we need to read up and S and N cycles
        // before we add them
        //cpu.cycle_tick(wait_states[static_cast<int>(mem_region)][sizeof(access_type) >> 1]);
    }
}






// gba is locked to little endian
template<typename access_type>
access_type Mem::read_rom(uint32_t addr)
{

    uint32_t len = rom.size();
    // while not illegal it probably means the there are errors
    if(((addr&0x1FFFFFF) + sizeof(access_type)) > len)
    {
        // unused
        return 0;
    }


    //return rom[addr - <whatever page start>];
    return handle_read<access_type>(rom,addr&0x1FFFFFF);        
}

// dont know how these work yet!
template<typename access_type>
access_type Mem::read_flash(uint32_t addr)
{
    UNUSED(addr);
    return 0;
}

template<typename access_type>
access_type Mem::read_sram(uint32_t addr)
{
    UNUSED(addr);
    return 0;
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
    //return oam[addr & 0x3ff];
    return handle_read<access_type>(oam,addr&0x3ff);   
}

template<typename access_type>
access_type Mem::read_vram(uint32_t addr)
{
    //return vram[addr-0x06000000];
    addr = (addr - 0x06000000) %  0x18000;
    return handle_read<access_type>(vram,addr);
}

template<typename access_type>
access_type Mem::read_pal_ram(uint32_t addr)
{
    //return pal_ram[addr & 0x3ff];
    return handle_read<access_type>(pal_ram,addr&0x3ff);
}

template<typename access_type>
access_type Mem::read_board_wram(uint32_t addr)
{
    //return board_wram[addr & 0x3ffff];
    return handle_read<access_type>(board_wram,addr&0x3ffff);
}

template<typename access_type>
access_type Mem::read_chip_wram(uint32_t addr)
{
    //return chip_wram[addr & 0x7fff];
    return handle_read<access_type>(chip_wram,addr&0x7fff);
}

template<typename access_type>
access_type Mem::read_bios(uint32_t addr)
{
    //return bios_rom[addr];
    return handle_read<access_type>(bios_rom,addr&0x3fff);
}

// dont know how these work
template<typename access_type>
void Mem::write_flash(uint32_t addr,access_type v)
{
    UNUSED(addr); UNUSED(v);
}



template<typename access_type>
void Mem::write_sram(uint32_t addr,access_type v)
{

    UNUSED(addr); UNUSED(v);
}

        

//access handler for reads (for non io mapped mem)
// need checks for endianess here for completeness
template<typename access_type>
void Mem::handle_write(std::vector<uint8_t> &buf,uint32_t addr,access_type v)
{

#ifdef BOUNDS_CHECK // bounds check the memory access (we are very screwed if this happens)
    if(buf.size() < addr + sizeof(access_type))
    {
        auto err = fmt::format("out of range handle write at: {:08x}\n",cpu.get_pc());
        throw std::runtime_error(err);
    }
#endif



    //(*(access_type*)(buf.data()+addr)) = v;
    memcpy(buf.data()+addr,&v,sizeof(access_type));
}


// should probably handle our "waitstate" timings for io by here

// as io has side effects we need to write to it byte by byte
template<>
void Mem::write_io<uint8_t>(uint32_t addr,uint8_t v)
{
    //io[addr & 0x3ff] = v;

    write_io_regs(addr,v);
}


template<>
void Mem::write_io<uint16_t>(uint32_t addr,uint16_t v)
{
    //io[addr & 0x3ff] = v;

    write_io_regs(addr,v&0x000000ff);
    write_io_regs(addr+1,(v&0x0000ff00) >> 8);
}


template<>
void Mem::write_io<uint32_t>(uint32_t addr,uint32_t v)
{
    //io[addr & 0x3ff] = v;

    write_io_regs(addr,v&0x000000ff);
    write_io_regs(addr+1,(v&0x0000ff00) >> 8); 
    write_io_regs(addr+2,(v&0x00ff0000) >> 16);
    write_io_regs(addr+3,(v&0xff000000) >> 24);
}



template<typename access_type>
void Mem::write_oam(uint32_t addr,access_type v)
{
    //oam[addr & 0x3ff] = v;
    handle_write<access_type>(oam,addr&0x3ff,v);
}

template<typename access_type>
void Mem::write_vram(uint32_t addr,access_type v)
{
    //vram[addr-0x06000000] = v;
     addr = (addr - 0x06000000) %  0x18000;
    handle_write<access_type>(vram,addr,v); 
}

template<typename access_type>
void Mem::write_pal_ram(uint32_t addr,access_type v)
{
    //pal_ram[addr & 0x3ff] = v;
    handle_write<access_type>(pal_ram,addr&0x3ff,v);
}

template<typename access_type>
void Mem::write_board_wram(uint32_t addr,access_type v)
{
    //return board_wram[addr & 0x3ffff] = v;
    handle_write<access_type>(board_wram,addr&0x3ffff,v);
}

template<typename access_type>
void Mem::write_chip_wram(uint32_t addr,access_type v)
{
    //chip_wram[addr & 0x7fff] = v;
    handle_write<access_type>(chip_wram,addr&0x7fff,v);
}

}