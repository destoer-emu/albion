#include <n64/n64.h>
#include <n64/mem_constants.h>

namespace nintendo64
{

void reset_mem(Mem &mem, const std::string &filename)
{
    // read rom in and hle the pif rom
    read_file(filename,mem.rom);

    // init memory
    // 8mb rd ram
    mem.rd_ram.resize(8 * 1024 * 1024);
    mem.sp_dmem.resize(0x1000);

    mem.sp_imem.resize(0x1000);

    const auto magic = handle_read<uint32_t>(mem.rom,0x0);

    // if rom is middle endian byteswap it

    if(magic != 0x80371240)
    {
        puts("byteswapping rom");
        std::iter_swap(mem.rom.begin(),mem.rom.end()-1);
    }

    // hle pif rom
    memcpy(mem.sp_dmem.data(),mem.rom.data(),0x1000);

    // ri
    mem.ri_select = 0x0;
    mem.ri_config = 0x0;
    mem.ri_base = 0x0;
    mem.ri_refresh = 0x0;

    // pi 
    mem.pi_cart_addr = 0;
    mem.pi_dram_addr = 0;
    mem.pi_status = 0;

    // MI
    mem.mi_mode = 0;
    mem.mi_intr = 0;
}

// TODO: this will probably have to be switched over to software page table
// come time for implementing the tlb but lets just keep things nice and simple for now
// and do a giant if else chain for everything
u32 remap_addr(u32 addr)
{
    if(addr < 0x80000000)
    {
        printf("KUSEG %8x\n",addr);
        exit(1);
    }

    // KSEG0 (direct mapped cached)
    else if(addr < 0xa0000000)
    {
        return addr - 0x80000000;
    }

    // KSEG1 (direct mapped uncached)
    else if(addr < 0xc0000000)
    {
        return addr - 0xa0000000;
    }

    // KSSEG
    else if(addr < 0xe0000000)
    {
        printf("KSSEG %8x\n",addr);
        exit(1);
    }

    else
    {
        printf("KSEG3 %8x\n",addr);
        exit(1);
    }

    assert(false);
    return 0;
}


// for now assume accesses are force aligned
// however they are supposed to throw exceptions
// when they are not

template<typename access_type>
access_type read_mem(N64 &n64, u32 addr)
{
    // force align addr
    addr &= ~(sizeof(access_type)-1);   

    UNUSED(n64);
    addr = remap_addr(addr);

    // just do something naive for now so we can get roms running
    if(addr < 0x00800000)
    {
        return handle_read<access_type>(n64.mem.rd_ram,addr);
    }

    // UNUSED
    else if(addr < 0x03F00000)
    {
        return 0;
    }

    else if(addr < 0x04000000)
    {
        // rdram config ignore for now
        return 0;
    /*
        switch(addr)
        {
            default:
            {
                unimplemented("read_mem: rdram regs %08x\n",addr);
                return 0;
            }
        }
    */
    }

    else if(addr < 0x04001000)
    {
        return handle_read<access_type>(n64.mem.sp_dmem,addr & 0xfff);
    }

    else if(addr < 0x04002000)
    {
        return handle_read<access_type>(n64.mem.sp_imem,addr & 0xfff);
    }

    // UNUSED
    else if(addr < 0x04040000)
    {
        return 0;
    }

    // TODO: start here impl the read lw is doing
    else if(addr < 0x04100000)
    {
        unimplemented("read_mem: sp regs");
        return 0;
    }

    else if(addr < 0x04200000)
    {
        unimplemented("read_mem: dp command regs");
        return 0;
    }

    else if(addr < 0x04300000)
    {
        unimplemented("read_mem: dp span regs");
        return 0;
    }

    else if(addr < 0x04400000)
    {
        switch(addr)
        {
            case MI_VERSION_REG: return static_cast<access_type>(0x02020102);

            default:
            {
                unimplemented("read_mem: mips interface: %8x\n",addr);
                return 0;
            }
        }
    }

    else if(addr < 0x04500000)
    {
        unimplemented("read_mem: video interface");
        return 0;
    }

    else if(addr < 0x04600000)
    {
        unimplemented("read_mem: audio interface");
        return 0;
    }

    else if(addr < 0x04700000)
    {
        switch(addr)
        {
            case PI_STATUS_REG: return n64.mem.pi_status;

            
            default:
            {
                unimplemented("read_mem: peripheral interface: %8x\n",addr);
                return 0;
            }
        }
    }
    
    else if(addr < 0x04800000)
    {
        // UNUSED
        if(addr >= 0x04700020)
        {
            return 0;
        }

        // TODO: should these all be byte accesses?
        switch(addr)
        {
            case RI_SELECT_REG: return n64.mem.ri_select;
            case RI_REFRESH_REG: return n64.mem.ri_refresh;

            default: unimplemented("read_mem: rdram interface: %8x\n",addr);
        }

        return 0;
    }

    else if(addr < 0x04900000)
    {
        unimplemented("serial interface");
        return 0;
    }

    // UNUSED
    else if(addr < 0x05000000)
    {
        return static_cast<access_type>(0xffffffff);
    }

    // n64dd
    else if(addr < 0x08000000)
    {
        // return as not present for now
        return static_cast<access_type>(0xffffffff);
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
        const auto rom_addr = 0x0fffffff;
        return handle_read<access_type>(n64.mem.rom, rom_addr & (n64.mem.rom.size()-1));
    }

    else
    {
        printf("read_mem: unknown physical address: %8x\n",addr);
        exit(1);
        return 0;
    }
}

// for now assume accesses are force aligned
// however they are supposed to throw exceptions
// when they are not

template<typename access_type>
void write_mem(N64 &n64, u32 addr, access_type v)
{
    // force align addr
    addr &= ~(sizeof(access_type)-1);   

    UNUSED(n64);
    addr = remap_addr(addr);

    // just do something naive for now so we can get roms running
    if(addr < 0x00800000)
    {
        handle_write<access_type>(n64.mem.rd_ram,addr,v);
    }

    // UNUSED
    else if(addr < 0x03F00000)
    {
    }

    else if(addr < 0x04000000)
    {
    /* RDRAM configuration (we will ignore this for now)

        switch(addr)
        {
            default: unimplemented("write_mem: rdram regs: %8x\n",addr); 
        }
    */
    }

    else if(addr < 0x04001000)
    {
        handle_write<access_type>(n64.mem.sp_dmem,addr & 0xfff,v);
    }

    else if(addr < 0x04002000)
    {
        handle_write<access_type>(n64.mem.sp_imem,addr & 0xfff,v);
    }

    // UNUSED
    else if(addr < 0x04040000)
    {

    }

    // TODO: start here impl the read lw is doing
    else if(addr < 0x04100000)
    {
        unimplemented("write_mem: sp regs");
    }

    else if(addr < 0x04200000)
    {
        unimplemented("write_mem: dp command regs");
    }

    else if(addr < 0x04300000)
    {
        unimplemented("write_mem: dp span regs");
    }

    else if(addr < 0x04400000)
    {
        switch(addr)
        {  
            case MI_MODE_REG: 
            {
                n64.mem.mi_mode = v; 
                if(is_set(v,11)) // bit 11 wrote clear DP interrupt bit 
                {
                    n64.mem.mi_intr = deset_bit(n64.mem.mi_intr,DP_INTR_BIT);
                }
                break;
            }

            default: unimplemented("write_mem: mips interface : %8x\n",addr); break;
        }
    }

    else if(addr < 0x04500000)
    {
        unimplemented("write_mem: video interface");
    }

    else if(addr < 0x04600000)
    {
        unimplemented("write_mem: audio interface");
    }

    else if(addr < 0x04700000)
    {
        switch(addr)
        {
            case PI_CART_ADDR_REG:
            {
                // aligned on 2 bytes
                n64.mem.pi_cart_addr = v;
                break;
            }

            case PI_CART_DRAM_ADDR_REG:
            {
                // aligned on 8 bytes
                n64.mem.pi_dram_addr = v & 0xffffff;
                break;
            }

            // need to find proper pi dma info
            // is this where we start a dma?
            case PI_WR_LEN_REG:
            {
                n64.mem.pi_wr_len = v & 0xffffff;
                printf("pi dma %08x:%08x:%08x\n",n64.mem.pi_cart_addr,n64.mem.pi_dram_addr,n64.mem.pi_wr_len + 1);
                exit(1);
                break;
            }

            default: unimplemented("write_mem: pi interface: %08x\n",addr);
        }
    }
    
    else if(addr < 0x04800000)
    {
        // UNUSED
        if(addr >= 0x04700020)
        {
            return;
        }



        switch(addr)
        {
            case RI_SELECT_REG: n64.mem.ri_select = v & 0b111111; break;
            case RI_CONFIG_REG: n64.mem.ri_config = v & 0b1111111; break;
            case RI_CURRENT_LOAD_REG: break; // write only ignore for now
            case RI_BASE_REG: n64.mem.ri_base = v & 0b1111; break;
            case RI_REFRESH_REG: n64.mem.ri_refresh = v & 0b1111111111111111111; break;

            default: unimplemented("write_mem: rdram interface: %8x\n",addr);
        }
    }

    else
    {
        std::cout << fmt::format("write_mem: unknown physical address: {:8x}:{:x}\n",addr,v);
        exit(1);
    }
}


u8 read_u8(N64 &n64,u32 addr)
{
    return read_mem<u8>(n64,addr);
}

u16 read_u16(N64 &n64,u32 addr)
{
    return bswap(read_mem<u16>(n64,addr));
}

u32 read_u32(N64 &n64,u32 addr)
{
    return bswap(read_mem<u32>(n64,addr));
}

u64 read_u64(N64 &n64,u32 addr)
{
    return bswap(read_mem<u64>(n64,addr));
}


void write_u8(N64 &n64,u32 addr,u8 v)
{
    write_mem<u8>(n64,addr,v);
}

void write_u16(N64 &n64,u32 addr,u16 v)
{
    write_mem<u16>(n64,addr,bswap(v));
}

void write_u32(N64 &n64,u32 addr,u32 v)
{
    write_mem<u32>(n64,addr,bswap(v));
}

void write_u64(N64 &n64,u32 addr,u64 v)
{
    write_mem<u64>(n64,addr,bswap(v));
}



}