#include <gba/gba.h>

namespace gameboyadvance
{

// template instantsation for our memory reads
template uint8_t Mem::read_mem<uint8_t>(uint32_t addr);
template uint16_t Mem::read_mem<uint16_t>(uint32_t addr);
template uint32_t Mem::read_mem<uint32_t>(uint32_t addr);

template uint8_t Mem::read_memt<uint8_t>(uint32_t addr);
template uint16_t Mem::read_memt<uint16_t>(uint32_t addr);
template uint32_t Mem::read_memt<uint32_t>(uint32_t addr);



template void Mem::write_mem<uint8_t>(uint32_t addr, uint8_t v);
template void Mem::write_mem<uint16_t>(uint32_t addr, uint16_t v);
template void Mem::write_mem<uint32_t>(uint32_t addr, uint32_t v);

template void Mem::write_memt<uint8_t>(uint32_t addr, uint8_t v);
template void Mem::write_memt<uint16_t>(uint32_t addr, uint16_t v);
template void Mem::write_memt<uint32_t>(uint32_t addr, uint32_t v);

template bool Mem::fast_memcpy<uint16_t>(uint32_t src, uint32_t dst, uint32_t n);
template bool Mem::fast_memcpy<uint32_t>(uint32_t src, uint32_t dst, uint32_t n);

template uint32_t Mem::get_waitstates<uint32_t>(uint32_t addr) const;
template uint32_t Mem::get_waitstates<uint16_t>(uint32_t addr) const;
template uint32_t Mem::get_waitstates<uint8_t>(uint32_t addr) const;


Mem::Mem(GBA &gba) : dma{gba}, debug(gba.debug), cpu(gba.cpu), 
    disp(gba.disp), apu(gba.apu), scheduler(gba.scheduler)
{
    // alloc our underlying system memory
    bios_rom.resize(0x4000);
    board_wram.resize(0x40000);
    chip_wram.resize(0x8000);
    pal_ram.resize(0x400);
    vram.resize(0x18000);
    oam.resize(0x400); 
    sram.resize(0x8000);
    rom.resize(32*1024*1024);
    std::fill(board_wram.begin(),board_wram.end(),0);
    std::fill(chip_wram.begin(),chip_wram.end(),0);
    std::fill(pal_ram.begin(),pal_ram.end(),0);
    std::fill(vram.begin(),vram.end(),0);
    std::fill(oam.begin(),oam.end(),0);
    std::fill(sram.begin(),sram.end(),0);


    backing_vec[static_cast<size_t>(memory_region::wram_board)] = board_wram.data();
    backing_vec[static_cast<size_t>(memory_region::wram_chip)] = chip_wram.data();
    backing_vec[static_cast<size_t>(memory_region::pal)] = pal_ram.data();
    backing_vec[static_cast<size_t>(memory_region::vram)] = vram.data();
    backing_vec[static_cast<size_t>(memory_region::oam)] = oam.data();
    backing_vec[static_cast<size_t>(memory_region::rom)] = rom.data();
}

void Mem::init(std::string filename)
{
    this->filename = filename;

    // read out rom
    read_file(filename,rom);

    

    std::fill(board_wram.begin(),board_wram.end(),0);
    std::fill(chip_wram.begin(),chip_wram.end(),0);
    std::fill(pal_ram.begin(),pal_ram.end(),0);
    std::fill(vram.begin(),vram.end(),0);
    std::fill(oam.begin(),oam.end(),0);
    std::fill(sram.begin(),sram.end(),0);

    
    // read out rom info here...

    // ok determine what kind of save type we are using :)
    // https://dillonbeliveau.com/2020/06/05/GBA-FLASH.html
    // cheers dillon :)

    // ok so we need to search the rom to see if there is a
    // string in there that can tell us the save type
    bool found = false;

    for(size_t i = 0; i < CART_TYPE_SIZE; i++)
    {
        const auto &s = cart_magic[i];

        // so now we need to do a byte search on the rom
        // and find the save type 
        if(std::search(rom.begin(),rom.end(),s.begin(),s.end()) != rom.end())
        {
            std::cout << "found save type: " << s << "\n";

            cart_type = save_region[i];
            save_size = save_sizes[i];

            for(int i = 0; i < 3; i++)
            {
                wait_states[static_cast<size_t>(memory_region::cart_backup)][i] = cart_wait_states[static_cast<size_t>(cart_type)][i];
            }
            found = true;
            break;
        }
    }

    if(!found)
    {
        puts("cart type not detected defaulting to flash!");
        cart_type = save_type::flash;
        save_size = 0x20000;
    }

    
    switch(cart_type)
    {
        case save_type::flash:
        {
            flash.init(save_size,filename);
            break;
        }

        case save_type::sram:
        {
            const auto save_name = get_save_file_name(filename);
            try
            {
                read_file(save_name,sram);
            } catch(std::exception &ex) {}


            break;
        }

        case save_type::eeprom:
        {
            std::fill(sram.begin(),sram.end(),0xff);
            const auto save_name = get_save_file_name(filename);
            try
            {
                read_file(save_name,sram);
            } catch(std::exception &ex) {}
            addr_size = -1;
            state = eeprom_state::ready;
            eeprom_idx = 0;
            eeprom_addr = 0;
            eeprom_command = 0;
            eeprom_data = 0;
            break;
        }
    }

    std::cout << "rom size: " << rom.size() << "\n";

    cart_ram_dirty = false;
    frame_count = 0;

    // read and copy in the bios rom
    read_file("GBA.BIOS",bios_rom);
    //read_file("gba_bios.bin",bios_rom);

    if(bios_rom.size() != 0x4000)
    {
        throw std::runtime_error("invalid bios size!");
    }
    mem_io.init();
    dma.init();
    
    memcpy(wait_states,wait_states_default,sizeof(wait_states));
    update_wait_states();

    // if we are not using the bios boot we need to set postflg
    mem_io.postflg = 1;   


    page_table.resize(16384);
    for(size_t i = 0; i < page_table.size(); i++)
    {
        uint32_t base = i * 0x4000;

        const auto mem_region = memory_region_table[(base >> 24) & 0xf];

        switch(mem_region)
        {
            // TODO switch this out when we enter it
            // so we dont actually have to constantly have this null
            // when we are fetching out of it
            case memory_region::bios:
            {
                page_table[i] = nullptr;
                break;
            }

            case memory_region::wram_board:
            {
                page_table[i] = &board_wram[base & 0x3ffff];
                break;
            }

            case memory_region::wram_chip:
            {
                page_table[i] = &chip_wram[base & 0x7fff];
                break;
            }

            case memory_region::io:
            {
                page_table[i] = nullptr;
                break;
            }

            case memory_region::pal:
            {
                page_table[i] = nullptr;
                break;
            }

            case memory_region::vram:
            {
                base = base & 0x1FFFF;
                if(base > 0x17fff)
                {
                    // align to 32k chunk
                    base = 0x10000 + (base & 0x7fff);
                }
                page_table[i] = &vram[base];
                break;
            }

            case memory_region::oam:
            {
                page_table[i] = nullptr;
                break;
            }

            case memory_region::rom:
            {
                if(is_eeprom(base))
                {
                    page_table[i] = nullptr;
                }

                else
                {
                    page_table[i] = &rom[base & 0x1FFFFFF];
                }
                break;
            }

            case memory_region::cart_backup:
            {
                page_table[i] = nullptr;
                break;
            }

            case memory_region::undefined:
            {
                page_table[i] = nullptr;
                break;
            }
        }
    }
}

void Mem::switch_bios(bool in_bios)
{
    if(in_bios)
    {
        page_table[0] = bios_rom.data();
    }

    else
    {
        page_table[0] = nullptr;
    }
}

void Mem::save_cart_ram()
{
    switch(cart_type)
    {
        case save_type::flash:
        {
            flash.save_ram();
            break;
        }

        case save_type::sram:
        {
            const auto save_name = get_save_file_name(filename);
            write_file(save_name,sram);
            break;
        }

        case save_type::eeprom:
        {
            const auto save_name = get_save_file_name(filename);
            write_file(save_name,sram);            
            break;
        }
    }    
}


void Mem::frame_end()
{
	if(cart_ram_dirty)
	{
		if(++frame_count >= FRAME_SAVE_LIMIT)
		{
			save_cart_ram();
			frame_count = 0;
			cart_ram_dirty = false;
		}
	}
}

// hack for soundbias to boot bios...
// fix when we properly impl the reg
uint16_t soundbias = 0;


void Mem::write_timer_control(int timer,uint8_t v)
{
    
    auto &t = cpu.cpu_io.timers[timer];
    t.write_control(v);

    // timer is enabled and count up not set 
    if(t.enable && !t.count_up)
    {
        cpu.insert_new_timer_event(timer);
    } 

    if(!t.enable)
    {
        const auto event_type = static_cast<gba_event>(timer+static_cast<int>(gba_event::timer0));
        scheduler.remove(event_type);
    }        
}


uint8_t Mem::read_timer_counter(int timer, int idx)
{
    const auto event_type = static_cast<gba_event>(timer+static_cast<int>(gba_event::timer0));
    const auto active = scheduler.is_active(event_type);

    // remove and reinsert event
    scheduler.remove(event_type);

    if(active)
    {
        cpu.insert_new_timer_event(timer);
    }

    return cpu.cpu_io.timers[timer].read_counter(idx);
}

void Mem::write_io_regs(uint32_t addr,uint8_t v)
{
    // io not mirrored bar one undocumented register
    if(addr >= 0x04000400)
    {
        return;
    }


    addr &= IO_MASK;


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
        case IO_BG2PA: disp.disp_io.bg2_scale_param.write_a(0,v); break;
        case IO_BG2PA+1: disp.disp_io.bg2_scale_param.write_a(1,v); break;

        case IO_BG2PB: disp.disp_io.bg2_scale_param.write_b(0,v); break;
        case IO_BG2PB+1: disp.disp_io.bg2_scale_param.write_b(1,v); break;

        case IO_BG2PC: disp.disp_io.bg2_scale_param.write_c(0,v); break;
        case IO_BG2PC+1: disp.disp_io.bg2_scale_param.write_c(1,v); break;

        case IO_BG2PD: disp.disp_io.bg2_scale_param.write_d(0,v); break;
        case IO_BG2PD+1: disp.disp_io.bg2_scale_param.write_d(1,v); break;

        // bg2 reference point
        case IO_BG2X_L: disp.disp_io.bg2_ref_point.write_x(0,v); break;
        case IO_BG2X_L+1: disp.disp_io.bg2_ref_point.write_x(1,v); break;
        case IO_BG2X_H: disp.disp_io.bg2_ref_point.write_x(2,v); break;
        case IO_BG2X_H+1: disp.disp_io.bg2_ref_point.write_x(3,v); break;

        case IO_BG2Y_L: disp.disp_io.bg2_ref_point.write_y(0,v); break;
        case IO_BG2Y_L+1: disp.disp_io.bg2_ref_point.write_y(1,v); break;
        case IO_BG2Y_H: disp.disp_io.bg2_ref_point.write_y(2,v); break;
        case IO_BG2Y_H+1: disp.disp_io.bg2_ref_point.write_y(3,v); break;


        // bg 3 scalaing / rotation params
        case IO_BG3PA: disp.disp_io.bg3_scale_param.write_a(0,v); break;
        case IO_BG3PA+1: disp.disp_io.bg3_scale_param.write_a(1,v); break;

        case IO_BG3PB: disp.disp_io.bg3_scale_param.write_b(0,v); break;
        case IO_BG3PB+1: disp.disp_io.bg3_scale_param.write_b(1,v); break;

        case IO_BG3PC: disp.disp_io.bg3_scale_param.write_c(0,v); break;
        case IO_BG3PC+1: disp.disp_io.bg3_scale_param.write_c(1,v); break;

        case IO_BG3PD: disp.disp_io.bg3_scale_param.write_d(0,v); break;
        case IO_BG3PD+1: disp.disp_io.bg3_scale_param.write_d(1,v); break;

        // bg3 reference point
        case IO_BG3X_L: disp.disp_io.bg3_ref_point.write_x(0,v); break;
        case IO_BG3X_L+1: disp.disp_io.bg3_ref_point.write_x(1,v); break;
        case IO_BG3X_H: disp.disp_io.bg3_ref_point.write_x(2,v); break;
        case IO_BG3X_H+1: disp.disp_io.bg3_ref_point.write_x(3,v); break;

        case IO_BG3Y_L: disp.disp_io.bg3_ref_point.write_y(0,v); break;
        case IO_BG3Y_L+1: disp.disp_io.bg3_ref_point.write_y(1,v); break;
        case IO_BG3Y_H: disp.disp_io.bg3_ref_point.write_y(2,v); break;
        case IO_BG3Y_H+1: disp.disp_io.bg3_ref_point.write_y(3,v); break;


        case IO_WIN0H: disp.disp_io.win0h.write(0,v); break;
        case IO_WIN0H+1: disp.disp_io.win0h.write(1,v); break;

        case IO_WIN1H: disp.disp_io.win1h.write(0,v); break;
        case IO_WIN1H+1: disp.disp_io.win1h.write(1,v); break;

        case IO_WIN0V: disp.disp_io.win0v.write(0,v); break;
        case IO_WIN0V+1: disp.disp_io.win0v.write(1,v); break;

        case IO_WIN1V: disp.disp_io.win1v.write(0,v); break;
        case IO_WIN1V+1: disp.disp_io.win1v.write(1,v); break;

        case IO_WININ: disp.disp_io.win_cnt.write(static_cast<int>(window_source::zero),v); break;
        case IO_WININ+1: disp.disp_io.win_cnt.write(static_cast<int>(window_source::one),v); break;

        case IO_WINOUT: disp.disp_io.win_cnt.write(static_cast<int>(window_source::out),v); break;
        case IO_WINOUT+1: disp.disp_io.win_cnt.write(static_cast<int>(window_source::obj),v); break;


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


        // (write only)
        case IO_MOSAIC: disp.disp_io.mosaic.write(0,v); break;
        case IO_MOSAIC+1: disp.disp_io.mosaic.write(1,v); break;
        // unused
        case IO_MOSAIC+2: break;
        case IO_MOSAIC+3:  break;

        case IO_BLDCNT: disp.disp_io.bld_cnt.write(0,v); break;
        case IO_BLDCNT+1: disp.disp_io.bld_cnt.write(1,v); break;

        case IO_BLDALPHA: disp.disp_io.eva = v & 0x1f; break;
        case IO_BLDALPHA+1: disp.disp_io.evb = v & 0x1f; break;

        // write only
        case IO_BLDY: disp.disp_io.evy = v & 0x1f; break;
        // unused
        case IO_BLDY+1: break;
        case IO_BLDY+2: break;
        case IO_BLDY+3: break;

        // timers

        case IO_TM0CNT_L: cpu.cpu_io.timers[0].write_counter(0,v); break;
        case IO_TM0CNT_L+1: cpu.cpu_io.timers[0].write_counter(1,v); break;
        case IO_TM0CNT_H: write_timer_control(0,v); break;
        case IO_TM0CNT_H+1: break; // upper byte not used

        case IO_TM1CNT_L: cpu.cpu_io.timers[1].write_counter(0,v); break;
        case IO_TM1CNT_L+1: cpu.cpu_io.timers[1].write_counter(1,v); break;
        case IO_TM1CNT_H: write_timer_control(1,v); break;

        case IO_TM1CNT_H+1: break; // upper byte not used

        case IO_TM2CNT_L: cpu.cpu_io.timers[2].write_counter(0,v); break;
        case IO_TM2CNT_L+1: cpu.cpu_io.timers[2].write_counter(1,v); break;
        case IO_TM2CNT_H: write_timer_control(2,v); break;
        case IO_TM2CNT_H+1: break; // upper byte not used

        case IO_TM3CNT_L: cpu.cpu_io.timers[3].write_counter(0,v); break;
        case IO_TM3CNT_L+1: cpu.cpu_io.timers[3].write_counter(1,v); break;
        case IO_TM3CNT_H: write_timer_control(3,v); break;
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

        case IO_SOUNDBIAS: soundbias = (soundbias & 0xff00) | v; break;
        case IO_SOUNDBIAS+1: soundbias = (soundbias & 0x00ff) | (v << 8); break;

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

        case SIOCNT:
        { 
            mem_io.siocnt.write(0,v);
            // interrupt hack should fire after
            // transfer end not instantly 
            if(mem_io.siocnt.start && mem_io.siocnt.irq)
            {
                cpu.request_interrupt(interrupt::serial);
            }
            mem_io.siocnt.start = false;
            break;
        }
        case SIOCNT+1: mem_io.siocnt.write(1,v); break;


        case IO_IME: cpu.cpu_io.ime = is_set(v,0); cpu.update_intr_status(); break;
        case IO_IME+1: case IO_IME+2: case IO_IME+3: break; // unused

        case IO_IE:
        { 
            cpu.cpu_io.interrupt_enable = (cpu.cpu_io.interrupt_enable & 0xff00) | v;
            cpu.update_intr_status(); 
            break;
        }

        case IO_IE+1: 
        {
            cpu.cpu_io.interrupt_enable = (cpu.cpu_io.interrupt_enable & 0x00ff) | ((v & 0x3f) << 8);
            cpu.update_intr_status();
            break;
        }

        case IO_IF: 
        {
            cpu.cpu_io.interrupt_flag = (cpu.cpu_io.interrupt_flag & 0xff00) & ~v;
            cpu.update_intr_status();
            break;
        }

        case IO_IF+1:
        {
            cpu.cpu_io.interrupt_flag = (cpu.cpu_io.interrupt_flag & 0x00ff) & ((~v & 0x3f) << 8);
            cpu.update_intr_status();
            break;
        }

        case IO_POSTFLG: mem_io.postflg = v; break;

        case IO_HALTCNT: cpu.cpu_io.halt_cnt.write(v);  cpu.handle_power_state(); break;

        // gamepak wait timings ignore for now
        case IO_WAITCNT: mem_io.wait_cnt.write(0,v); update_wait_states(); break;
        case IO_WAITCNT+1: mem_io.wait_cnt.write(1,v); update_wait_states(); break;
        case IO_WAITCNT+2: break;
        case IO_WAITCNT+3: break;


        default: // here we will handle open bus when we have all our io regs done :)
        { 
            //auto err = fmt::format("[io {:08x}] unhandled write at {:08x}:{:x}",cpu.get_pc(),addr,v);
            //throw std::runtime_error(err);
            break;
        }
    }
}



uint8_t Mem::read_io_regs(uint32_t addr)
{
    // todo optimise this
    if(addr >= 0x04000400)
    {
        return 0;
    }

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

        case IO_WININ: return disp.disp_io.win_cnt.read(static_cast<int>(window_source::zero));
        case IO_WININ+1: return disp.disp_io.win_cnt.read(static_cast<int>(window_source::one));

        case IO_WINOUT: return disp.disp_io.win_cnt.read(static_cast<int>(window_source::out));
        case IO_WINOUT+1: return disp.disp_io.win_cnt.read(static_cast<int>(window_source::obj)); 

        case IO_BLDCNT: return disp.disp_io.bld_cnt.read(0);
        case IO_BLDCNT+1: return disp.disp_io.bld_cnt.read(1);

        case IO_BLDALPHA: return disp.disp_io.eva;
        case IO_BLDALPHA+1: return disp.disp_io.evb;

        // bg cnt
        case IO_BG0CNT: return disp.disp_io.bg_cnt[0].read(0);
        case IO_BG0CNT+1: return disp.disp_io.bg_cnt[0].read(1);

        case IO_BG1CNT: return disp.disp_io.bg_cnt[1].read(0);
        case IO_BG1CNT+1: return disp.disp_io.bg_cnt[1].read(1);

        case IO_BG2CNT: return disp.disp_io.bg_cnt[2].read(0);
        case IO_BG2CNT+1: return disp.disp_io.bg_cnt[2].read(1);

        case IO_BG3CNT: return disp.disp_io.bg_cnt[3].read(0);
        case IO_BG3CNT+1: return disp.disp_io.bg_cnt[3].read(1);

        // timers

        case IO_TM0CNT_L: return read_timer_counter(0,0);
        case IO_TM0CNT_L+1: return read_timer_counter(0,1);
        case IO_TM0CNT_H: return cpu.cpu_io.timers[0].read_control(); 
        case IO_TM0CNT_H+1: return 0; // upper byte not used

        case IO_TM1CNT_L: return read_timer_counter(1,0);
        case IO_TM1CNT_L+1: return read_timer_counter(1,1); 
        case IO_TM1CNT_H: return cpu.cpu_io.timers[1].read_control();
        case IO_TM1CNT_H+1: return 0; // upper byte not used

        case IO_TM2CNT_L: return read_timer_counter(2,0);
        case IO_TM2CNT_L+1: return read_timer_counter(2,1);
        case IO_TM2CNT_H: return cpu.cpu_io.timers[2].read_control(); 
        case IO_TM2CNT_H+1: return 0; // upper byte not used

        case IO_TM3CNT_L: return read_timer_counter(3,0);
        case IO_TM3CNT_L+1: return read_timer_counter(3,1);
        case IO_TM3CNT_H: return cpu.cpu_io.timers[3].read_control(); 
        case IO_TM3CNT_H+1: return 0; // upper byte not used


        case IO_SOUNDBIAS: return soundbias;
        case IO_SOUNDBIAS+1: return soundbias >> 8;

        case IO_IME: return cpu.cpu_io.ime;
        case IO_IME+1: case IO_IME+2: case IO_IME+3: return 0; // stub

        case IO_IE: return cpu.cpu_io.interrupt_enable & 0xff;
        case IO_IE+1: return (cpu.cpu_io.interrupt_enable >> 8) & 0x3f; 

        case IO_IF: return cpu.cpu_io.interrupt_flag & 0xff;
        case IO_IF+1: return (cpu.cpu_io.interrupt_flag >> 8) & 0x3f;         

        // gamepak wait timings ignore for now
        case IO_WAITCNT: return mem_io.wait_cnt.read(0);
        case IO_WAITCNT+1: return mem_io.wait_cnt.read(1); 
        case IO_WAITCNT+2: return  0; 
        case IO_WAITCNT+3: return 0; 

        case SIOCNT: return mem_io.siocnt.read(0);  
        case SIOCNT+1: return mem_io.siocnt.read(0);

        case IO_POSTFLG: return mem_io.postflg;

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


template<typename access_type>
uint32_t align_addr(uint32_t addr)
{
    // only allow up to 32bit
    static_assert(sizeof(access_type) <= 4);

    // 28 bit bus
    addr &= 0x0fffffff;

    // handle address alignment
    addr &= ~(sizeof(access_type)-1);    

    return addr;
}


template<typename access_type>
access_type Mem::read_mem_handler(uint32_t addr)
{

    const auto page = addr >> 14;
    if(page_table[page] != nullptr)
    {
        access_type v;
        const uint8_t *buf = page_table[page] + (addr & (0x4000-1));
        memcpy(&v,buf,sizeof(v));
        return v;
    }

    const auto mem_region = memory_region_table[(addr >> 24) & 0xf];

    switch(mem_region)
    {
        case memory_region::bios: 
        {
            // cant read from bios when not executing in it
            if(cpu.is_in_bios())
            //if(cpu.get_pc() < 0x4000)
            {
                return read_bios<access_type>(addr);
            }

            else // approximation for open bus
            {
                return cpu.get_pipeline_val();
            }
        }
        case memory_region::wram_board: return read_board_wram<access_type>(addr);
        case memory_region::wram_chip: return read_chip_wram<access_type>(addr);
        case memory_region::io: return read_io<access_type>(addr);
        case memory_region::pal: return read_pal_ram<access_type>(addr);
        case memory_region::vram: return read_vram<access_type>(addr);
        case memory_region::oam: return read_oam<access_type>(addr);
        case memory_region::rom: 
        {
            const auto eeprom = is_eeprom(addr);

            if(state == eeprom_state::read_active && eeprom)
            {
                return read_eeprom();
            }

            else if(eeprom && state == eeprom_state::ready)
            {
                return 1;
            }

            else
            {
                return read_rom<access_type>(addr);
            }
            return 0;
        }
        // flash is also accesed here
        // we should really switch over to fptrs so this is nicer to swap stuff out
        case memory_region::cart_backup:
        { 
            // this should probably just be a func pointer
            // but this is fine for now
            switch(cart_type)
            {
                case save_type::flash:
                {
                    return flash.read_flash(addr);
                }

                // byte access only check what happens when
                // a non byte access is attempted...
                case save_type::sram:
                {
                    return sram[addr & 0x7fff];
                }

                case save_type::eeprom:
                {
                    break;
                }
            } 

            return 0;
        }

        default: // approximation for open bus
        case memory_region::undefined: return cpu.get_pipeline_val(); 
    }
}

// unused memory is to be ignored

 // read mem unticked
template<typename access_type>
access_type Mem::read_mem(uint32_t addr)
{
    addr = align_addr<access_type>(addr);

#ifdef DEBUG
    const auto v = read_mem_handler<access_type>(addr);
    if(debug.breakpoint_hit(addr,v,break_type::read))
    {
        write_log(debug,"write breakpoint hit at {:08x}:{:08x}:{:08x}",addr,v,cpu.get_pc());
        debug.halt();
    }
    return v;
#else    
    return read_mem_handler<access_type>(addr);    
#endif
}


// timed memory access
template<typename access_type>
access_type Mem::read_memt(uint32_t addr)
{
    access_type v = read_mem<access_type>(addr);
    tick_mem_access<access_type>(addr);
    return v;
}

bool Mem::is_eeprom(uint32_t addr) const
{
    return cart_type == save_type::eeprom
        && (addr >= 0x0DFFFF00 || ((rom.size() < 32*1024*1024) && (addr >= 0x0D000000))) 
        && addr <= 0x0DFFFFFF;
}


uint8_t Mem::read_eeprom()
{
    // return garbage
    if(eeprom_idx < 4)
    { 
        eeprom_idx += 1;
        return 1;
    }

    // actually return this out of sram
    const bool bit = is_set(eeprom_data,63);
    eeprom_data <<= 1;
    eeprom_idx += 1;
    if(eeprom_idx == 68)
    {
        state = eeprom_state::ready;
        //puts("command done");
        eeprom_idx = 0;
        eeprom_command = 0;
        eeprom_addr = 0;
        eeprom_data = 0;
    }
    return bit;
    
}



void Mem::write_eeprom(uint8_t v)
{
    // ok check for an active dma3 xfer
    // then we will use the word size to get the addr length
    if(addr_size == -1)
    {
        if(dma.active_dma == 3)
        {
            const auto reg = dma.dma_regs[3];
            if(is_eeprom(reg.dst_shadow))
            {
                addr_size = reg.word_count_shadow - 3;
                if(addr_size != 6 && addr_size != 14)
                {
                    //printf("warning unknown eeprom addr size: %d\n",addr_size);
                    addr_size = -1;
                }
            }
        }
    }

    if(addr_size == -1)
    {
        return;
    }

    switch(state)
    {
        case eeprom_state::ready:
        {
            if(addr_size == -1)
            {
                break;
            }

            eeprom_command <<= 1;
            eeprom_command |= v & 1;
            eeprom_idx += 1;

            //printf("read write: %d\n",v & 1);

            if(eeprom_idx == 2)
            {
                const auto command = (eeprom_command & 3);
                if(command == 0b11)
                {
                    //puts("read setup"); // why are we gettigng a 2nd read command?
                    state = eeprom_state::read_setup;
                }

                else if(command == 0b10)
                {
                    //puts("write setup");
                    state = eeprom_state::write_setup;
                }

                else
                {
                    //printf("warning invalid eeprom command state: %d\n",eeprom_command);
                }

                eeprom_idx = 0;
                eeprom_command = 0;
                eeprom_addr = 0;  
                eeprom_data = 0;    
            }
            break;
        }

        case eeprom_state::write_setup:
        {
            if(eeprom_idx < addr_size)
            {
                eeprom_addr <<= 1;
                eeprom_addr |= v & 1;
                eeprom_idx += 1;
            }

            // this aint triggering when it should
            else if(eeprom_idx < addr_size + 64)
            {
                const auto set = v & 1;
                //printf("%d\n",set);
                // data off this is wrong check msb
                eeprom_data <<= 1;
                eeprom_data |= set;
                eeprom_idx += 1;
            }

            // transfer end
            else
            {
                handle_write<uint64_t>(sram,eeprom_addr * 8, eeprom_data);
                //printf("write end %08x: %016lx\n",eeprom_addr,eeprom_data);
                state = eeprom_state::ready;
                eeprom_idx = 0;
                eeprom_command = 0;
                eeprom_addr = 0;
                eeprom_data = 0;
            }
            break;
        }

        case eeprom_state::read_setup:
        {
            eeprom_addr <<= 1;
            eeprom_addr |= v & 1;
            eeprom_idx += 1;
            if(eeprom_idx == addr_size)
            {
                state = eeprom_state::read_active;
                eeprom_data = handle_read<uint64_t>(sram,eeprom_addr * 8);
                //printf("read_addr: %08x: %016lx\n",eeprom_addr,eeprom_data);
                eeprom_idx = 0;
            }
            break;
        }
        // ignore this
        case eeprom_state::read_active:
        {
            break;
        }
    }    
}

// write mem
 // write mem unticked
template<typename access_type>
void Mem::write_mem(uint32_t addr,access_type v)
{

    addr = align_addr<access_type>(addr);

    const auto mem_region = memory_region_table[(addr >> 24) & 0xf];
#ifdef DEBUG
    if(debug.breakpoint_hit(addr,v,break_type::write))
    {
        write_log(debug,"write breakpoint hit at {:08x}:{:08x}:{:08x}",addr,v,cpu.get_pc());
        debug.halt();
    }   
#endif

    switch(mem_region)
    {
        
        case memory_region::bios:
        {
        /*
            // logging hack
            static int idx = 0;
            if(addr == 0x4)
            {
                printf("%d: ",idx++);
                for(int i = (sizeof(v) * 8) - 1; i >= 0; i--)
                {
                    printf("%d",is_set(v,i));
                }
                putchar('\n');
            }
        */
            break; // read only
        }
        case memory_region::wram_board: write_board_wram<access_type>(addr,v); break;
        case memory_region::wram_chip: write_chip_wram<access_type>(addr,v); break;
        case memory_region::io: write_io<access_type>(addr,v); break;
        case memory_region::pal: write_pal_ram<access_type>(addr,v); break;
        case memory_region::vram: write_vram<access_type>(addr,v); break;
        case memory_region::oam: write_oam<access_type>(addr,v); break;
        case memory_region::rom:
        {
            const auto eeprom = is_eeprom(addr);
            // check for eeprom access
            if(eeprom)
            {
                write_eeprom(v);
            }
            break;
        } 
        

        // flash is also accessed here 
        // we will have to set mem_region by hand when it is
        case memory_region::cart_backup:
        { 
            switch(cart_type)
            {
                case save_type::flash:
                {
                    flash.write_flash(addr,v);
                    cart_ram_dirty = true;
                    break;
                }

                // byte access only check what happens when
                // a non byte access is attempted...
                case save_type::sram:
                {
                    sram[addr & 0x7fff] = v;
                    cart_ram_dirty = true;
                    break;
                }

                case save_type::eeprom:
                {
                    break;
                }
            }
            break;
        }

        case memory_region::undefined: break;
    }

}

// ticked access
template<typename access_type>
void Mem::write_memt(uint32_t addr,access_type v)
{
    write_mem<access_type>(addr,v);
    tick_mem_access<access_type>(addr);
}





// we also need to a test refactor using one lib, constants and compiling in one dir
// and impl a basic timing test
// this is completly botched...
void set_wait(int *buf, int wait)
{
    buf[0] =  wait + 1;
    buf[1] =  wait + 1;
    buf[2] =  (wait * 2) + 1;
}

void Mem::update_wait_states()
{
    static constexpr int wait_first_table[] = {4,3,2,8};
    const auto &wait_cnt = mem_io.wait_cnt;

    // TODO: hack for prefetch if prefetch is enabled make access instant
    if(wait_cnt.prefetch)
    {
        for(int x = 0; x < 3; x++)
        {
            for(int y = 0; y < 2; y++)
            {
                for(int z = 0; z < 3; z++)
                {
                    rom_wait_states[x][y][z] = 1;
                }
            }
        }
    }

    const auto wait_first0 = wait_first_table[wait_cnt.wait01];
    const auto wait_second0 = wait_cnt.wait02? 2 : 1;
    set_wait(&rom_wait_states[0][0][0],wait_first0);
    set_wait(&rom_wait_states[0][1][0],wait_second0);

    const auto wait_first1 = wait_first_table[wait_cnt.wait11];
    const auto wait_second1 = wait_cnt.wait12? 4 : 1;
    set_wait(&rom_wait_states[1][0][0],wait_first1);
    set_wait(&rom_wait_states[1][1][0],wait_second1);


    const auto wait_first2 = wait_first_table[wait_cnt.wait21];
    const auto wait_second2 = wait_cnt.wait22? 8 : 1;
    set_wait(&rom_wait_states[2][0][0],wait_first2);
    set_wait(&rom_wait_states[2][1][0],wait_second2);
}



template<typename access_type>
uint32_t Mem::get_waitstates(uint32_t addr) const
{
    static_assert(sizeof(access_type) <= 4);

    // need to re pull the region incase dma triggered reads
    const int region =  (addr >> 24) & 0xf; 
    const auto mem_region = memory_region_table[region];
    
    switch(mem_region)
    {
        case memory_region::undefined:
        {
            // how long should this take?
            return 1;
        }

        // need to lookup waitstaes in seperate table for rom
        case memory_region::rom:
        {
            // hardcode to sequential access!
            return rom_wait_states[(region - 8) / 2][1][sizeof(access_type) >> 1];
        }

        // should unmapped addresses still tick a cycle?
        default:
        {
            // access type >> 1 to get the value
            // 4 -> 2 (word)
            // 2 -> 1 (half)
            // 1 -> 0 (byte)
            return wait_states[region][sizeof(access_type) >> 1];
        }
    }
}

// this is very hacky to get around our shoddy timings atm
// Emerald breaks in battle under this though im not sure timings is the reason
// metroid also showing lines, need to debug the gfx issues on the save files too
// it might be an issue with the pipeline...
template<typename access_type>
void Mem::tick_mem_access(uint32_t addr)
{
    //cpu.cycle_tick(1);


    // only allow up to 32bit
    static_assert(sizeof(access_type) <= 4);
    cpu.cycle_tick(get_waitstates<access_type>(addr));
}






// gba is locked to little endian
template<typename access_type>
access_type Mem::read_rom(uint32_t addr)
{
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
    return read_io_regs(addr)
        | read_io_regs(addr+1) << 8;
}

template<>
uint32_t Mem::read_io<uint32_t>(uint32_t addr)
{
    return read_io_regs(addr)
        | read_io_regs(addr+1) << 8
        | read_io_regs(addr+2) << 16
        | read_io_regs(addr+3) << 24;
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
    addr = addr & 0x1FFFF;
    if(addr > 0x17fff)
    {
        // align to 32k chunk
        addr = 0x10000 + (addr & 0x7fff);
    }

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
    // 8bit write restricted
    if constexpr(std::is_same<access_type,uint8_t>())
    {

    }

    else
    {
        //oam[addr & 0x3ff] = v;
        handle_write<access_type>(oam,addr&0x3ff,v);
    }
}



template<typename access_type>
void Mem::write_vram(uint32_t addr,access_type v)
{
    addr = addr & 0x1FFFF;
    if(addr > 0x17fff)
    {
        // align to 32k chunk
        addr = 0x10000 + (addr & 0x7fff);
    }

    // 8bit write does weird stuff depending on address
    if constexpr(std::is_same<access_type,uint8_t>())
    {
        const bool is_bitmap = disp.disp_io.disp_cnt.bg_mode >= 3;
        // data written to upper and lower halfword
        if(is_bitmap && addr < 0x14000)
        {
            vram[addr & ~1] = v;
            vram[(addr & ~1) + 1] = v;
        }

        else if(!is_bitmap && addr < 0x10000)
        {
            vram[addr & ~1] = v;
            vram[(addr & ~1) + 1] = v;
        }
        // else we dont care
        return;
    }

    else
    {
        //vram[addr-0x06000000] = v;
        handle_write<access_type>(vram,addr,v); 
    }
}

template<typename access_type>
void Mem::write_pal_ram(uint32_t addr,access_type v)
{
    addr &= 0x3ff;

    // 8bit write causes data to wrote to both bytes
    // of the accessed halfword
    if constexpr(std::is_same<access_type,uint8_t>())
    {
        pal_ram[addr & ~1] = v;
        pal_ram[(addr & ~1) + 1] = v;
    }

    else
    {
        //pal_ram[addr & 0x3ff] = v;
        handle_write<access_type>(pal_ram,addr,v);
    }
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


uint32_t Mem::align_addr_to_region(uint32_t addr) const
{
    const auto region = static_cast<int>(memory_region_table[(addr >> 24) & 0xf]);
    return addr & region_info[region].mask;
}

bool Mem::can_fast_memcpy(uint32_t dst, uint32_t src, uint32_t bytes) const
{
    const auto src_reg = memory_region_table[(src >> 24) & 0xf];
    const auto dst_reg = memory_region_table[(dst >> 24) & 0xf];

    if(src_reg == memory_region::io || src_reg == memory_region::cart_backup || 
        src_reg == memory_region::bios || src_reg == memory_region::undefined)
    {
        return false;
    }

    if(dst_reg == memory_region::io || dst_reg == memory_region::cart_backup || 
        dst_reg == memory_region::bios || dst_reg == memory_region::undefined || dst_reg == memory_region::rom) 
    {
        return false;
    }

    if(is_eeprom(src))
    {
        return false;
    }

    const auto src_offset = align_addr_to_region(src);
    const auto dst_offset = align_addr_to_region(dst);

    const auto src_size = region_info[static_cast<int>(src_reg)].size;
    const auto dst_size = region_info[static_cast<int>(dst_reg)].size;

    const bool in_range = (src_offset+bytes <= src_size && dst_offset+bytes <= dst_size);
/*
    if(!in_range)
    {
        printf("[%08x] not in region bounds: %08x:%08x:%08x:%08x\n",bytes,src_offset+bytes,dst_offset+bytes,src_size,dst_size);
        printf("%08x:%08x\n",src,dst);
    }
*/
    return in_range;
}

template<typename access_type>
bool Mem::fast_memcpy(uint32_t dst, uint32_t src, uint32_t n)
{
    static_assert(sizeof(access_type) >= 2);

    src = align_addr<access_type>(src);
    dst = align_addr<access_type>(dst);

    const auto bytes = n*sizeof(access_type);

    if(!can_fast_memcpy(dst,src,bytes))
    {
        return false;
    }


    

    const auto src_reg = memory_region_table[(src >> 24) & 0xf];
    const auto dst_reg = memory_region_table[(dst >> 24) & 0xf];

    uint8_t *src_ptr = backing_vec[static_cast<size_t>(src_reg)];
    uint8_t *dst_ptr = backing_vec[static_cast<size_t>(dst_reg)];

    assert(src_ptr != nullptr);
    assert(dst_ptr != nullptr);

    const auto src_offset = align_addr_to_region(src);
    const auto dst_offset = align_addr_to_region(dst);

    memcpy(dst_ptr+dst_offset,src_ptr+src_offset,bytes);  

    const auto src_wait = get_waitstates<access_type>(src);
    const auto dst_wait = get_waitstates<access_type>(dst);

    for(size_t i = 0; i < n; i++)
    {
        cpu.cycle_tick(src_wait);
        cpu.cycle_tick(dst_wait);
    }

    return true; 
}

}
