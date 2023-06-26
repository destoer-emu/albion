

namespace nintendo64
{

template<typename access_type>
access_type read_physical(N64 &n64, u32 addr)
{
    // just do something naive for now so we can get roms running
    if(addr < 0x00800000)
    {
        return handle_read_n64<access_type>(n64.mem.rd_ram,addr);
    }

    // UNUSED
    else if(addr < 0x03F00000)
    {
        return 0;
    }

    else if(addr < 0x04000000)
    {
        return read_rdram_regs(n64,addr);
    }

    else if(addr < 0x04001000)
    {
        return handle_read_n64<access_type>(n64.mem.sp_dmem,addr & 0xfff);
    }

    else if(addr < 0x04002000)
    {
        return handle_read_n64<access_type>(n64.mem.sp_imem,addr & 0xfff);
    }

    // UNUSED
    else if(addr < 0x04040000)
    {
        return 0;
    }


    else if(addr < 0x0500'0000)
    {
        const u32 idx = (addr >> 20) & 0xf;
        
        switch(idx)
        {
            case 0: return read_sp_regs(n64,addr); 
            case 1: unimplemented("read_mem: dp command regs"); return 0;
            case 2: unimplemented("read_mem: dp span regs"); return 0;
            case 3: return read_mi(n64,addr); 
            case 4: return read_vi(n64,addr); 
            case 5: unimplemented("read_mem: audio interface %08x\n",addr); return 0;
            case 6: return read_pi(n64,addr); 
            case 7: return read_ri(n64,addr);
            case 8: unimplemented("read_mem: serial interface :%08x\n",addr);  return 0;

            default: return 0;
        }
    }

    // n64dd
    else if(addr < 0x08000000)
    {
        // return as not present for now
        return access_type(0xffffffff);
    }
  
    // sram
    else if(addr < 0x10000000)
    {
        unimplemented("sram");
        return 0;
    }

    // rom
    else if(addr < 0x1FC00000)
    {
        const auto rom_addr = addr & (n64.mem.rom.size() - 1);
        return handle_read_n64<access_type>(n64.mem.rom, rom_addr);
    }

    // should this be ignored
    else if(addr < 0x1FC007C0)
    {
        unimplemented("pif boot rom read");
        return 0;
    }

    // pif ram (ignore for now but need to emulate later)
    else if(addr < 0x1FC00800)
    {
        unimplemented("pif ram");
        return 0;
    }

    else if(addr >= 0x1FD00000 && addr <= 0x7FFFFFFF)
    {
        unimplemented("cart domain 3");
        return 0;
    }

    else
    {
        printf("warning unused read at %08x\n",addr);
        return access_type(0xffff'ffff);
    }  
}


template<typename access_type>
void write_physical(N64 &n64, u32 addr, access_type v)
{
    // just do something naive for now so we can get roms running
    if(addr < 0x0080'0000)
    {
        handle_write_n64<access_type>(n64.mem.rd_ram,addr,v);
    }

    // UNUSED
    else if(addr < 0x03F0'0000)
    {
    }

    else if(addr < 0x0400'0000)
    {
        write_rdram_regs(n64,addr,v);
    }

    else if(addr < 0x0400'1000)
    {
        handle_write_n64<access_type>(n64.mem.sp_dmem,addr & 0xfff,v);
    }

    else if(addr < 0x0400'2000)
    {
        handle_write_n64<access_type>(n64.mem.sp_imem,addr & 0xfff,v);
    }

    // UNUSED
    else if(addr < 0x0404'0000)
    {

    }

    else if(addr < 0x0500'0000)
    {
        const u32 idx = (addr >> 20) & 0xf;
        
        switch(idx)
        {
            case 0: write_sp_regs(n64,addr,v); break;
            case 1: unimplemented("write_mem: dp command regs"); break;
            case 2: unimplemented("write_mem: dp span regs"); break;
            case 3: write_mi(n64,addr,v); break;
            case 4: write_vi(n64,addr,v); break;
            case 5: unimplemented("write_mem: audio interface %08x\n",addr); break;
            case 6: write_pi(n64,addr,v); break;
            case 7: write_ri(n64,addr,v); break;
            case 8: unimplemented("write_mem: serial interface :%08x\n",addr); break;

            default: break;
        }
    }


    // n64dd
    else if(addr < 0x08000000)
    {
       // not present for now
    }
  
    // sram
    else if(addr < 0x10000000)
    {
        unimplemented("sram");
    }

    // rom
    else if(addr < 0x1FC00000)
    {
        // does this do anything?
        unimplemented("rom write");
    }

    // should this be ignored
    else if(addr < 0x1FC007C0)
    {
        unimplemented("pif boot rom write");
    }

    // pif ram (ignore for now but need to emulate later)
    else if(addr < 0x1FC00800)
    {
        unimplemented("pif ram");
    }

    else if(addr >= 0x1FD00000 && addr <= 0x7FFFFFFF)
    {
        unimplemented("cart domain 3");
    }

    else
    {
        printf("warning unused write at %08x\n",addr);
    }   
}

}