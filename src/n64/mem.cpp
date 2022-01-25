#include <n64/n64.h>
#include <n64/mem_constants.h>

namespace nintendo64
{

void do_pi_dma(N64 &n64, u32 src, u32 dst, u32 len);


inline u64 swap_word(u64 v)
{
    return ((v & 0xffffffff) << 32)  | (v >> 32);
}

template<typename access_type>
access_type handle_read_n64(std::vector<u8> &buf, u32 addr)
{
    // handle endianess, we have swapped the entire rom
    // so offset the addresses
    if constexpr(sizeof(access_type) == 2)
    {
        addr ^= 2;
    }

    else if constexpr(sizeof(access_type) == 1)
    {
        addr ^= 3;
    }


    auto v = handle_read<access_type>(buf,addr);

    // 8 byte so we have to swap them
    if constexpr(sizeof(access_type) == 8)
    {
        v = swap_word(v);
    }


    return v;
}

template<typename access_type>
void handle_write_n64(std::vector<u8> &buf, u32 addr, access_type v)
{
    // handle endianess, we have swapped the entire rom
    // so offset the addresses
    if constexpr(sizeof(access_type) == 2)
    {
        addr ^= 2;
    }

    else if constexpr(sizeof(access_type) == 1)
    {
        addr ^= 3;
    }

    // 8 byte so swap words
    if constexpr(sizeof(access_type) == 8)
    {
        v = swap_word(v);
    }


    handle_write<access_type>(buf,addr,v);
}


void reset_mem(Mem &mem, const std::string &filename)
{
    // read rom in and hle the pif rom
    read_file(filename,mem.rom);

    if(mem.rom.size() < 32 * 1024 * 1024)
    {
        // ensure rom is power of two!!
        mem.rom.resize(32 * 1024 * 1024);
    }

    else
    {
        unimplemented("large rom");
    }



    // init memory
    // 8mb rd ram
    mem.rd_ram.resize(8 * 1024 * 1024);
    mem.sp_dmem.resize(0x1000);

    mem.sp_imem.resize(0x1000);

    const auto magic = handle_read<u32>(mem.rom,0x0);

    // if rom is middle endian byteswap it

    if(magic != 0x80371240)
    {
        puts("byteswapping rom");
        std::iter_swap(mem.rom.begin(),mem.rom.end()-1);
    }

    for(u32 i = 0; i < mem.rom.size(); i += sizeof(u32))
    {
        u32 v = handle_read<u32>(mem.rom,i);
        v = bswap(v);
        handle_write<u32>(mem.rom,i,v);
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

    // vi
    mem.vi_bpp = 0;
    mem.vi_gamma_dither = 0;
    mem.vi_gamma = 0;
    mem.vi_divot = 0;
    mem.vi_serrate = 0;
    mem.vi_aa = 0;

    mem.vi_origin = 0;
    mem.vi_width = 0;
    mem.vi_intr = 0;
    mem.vi_burst = 0;
    mem.vi_vsync = 0;
    mem.vi_hsync = 0;
    mem.vi_leap = 0;
    mem.vi_hstart = 0;
    mem.vi_vstart = 0;
    mem.vi_vburst = 0;
    mem.vi_xscale = 0;
    mem.vi_yscale = 0;
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
            case MI_VERSION_REG: return access_type(0x02020102);

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
        return access_type(0xffffffff);
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
access_type read_mem(N64 &n64, u32 addr)
{
    // force align addr
    addr &= ~(sizeof(access_type)-1);   

    addr = remap_addr(addr);

    return read_physical<access_type>(n64,addr);
}


template<typename access_type>
void write_physical(N64 &n64, u32 addr, access_type v)
{
    // just do something naive for now so we can get roms running
    if(addr < 0x00800000)
    {
        handle_write_n64<access_type>(n64.mem.rd_ram,addr,v);
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
        handle_write_n64<access_type>(n64.mem.sp_dmem,addr & 0xfff,v);
    }

    else if(addr < 0x04002000)
    {
        handle_write_n64<access_type>(n64.mem.sp_imem,addr & 0xfff,v);
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
        switch(addr)
        {
            case VI_CONTROL_REG:
            {
                n64.mem.vi_bpp = v & 0b11;
                n64.mem.vi_gamma_dither = is_set(v,2);
                n64.mem.vi_gamma = is_set(v,3);
                n64.mem.vi_divot = is_set(v,4);
                n64.mem.vi_serrate = is_set(v,6);
                n64.mem.vi_aa = (v >> 8) & 0b11;

                if(n64.mem.vi_bpp == 2)
                {
                    unimplemented("rgb 5551");
                }

                if(n64.mem.vi_gamma_dither)
                {
                    unimplemented("dither");
                }

                if(n64.mem.vi_gamma)
                {
                    unimplemented("gamma");
                }

                if(n64.mem.vi_divot)
                {
                    unimplemented("divot");
                }

                if(n64.mem.vi_serrate)
                {
                    unimplemented("serrate");
                }

                break;
            }

            case VI_ORIGIN_REG:
            {
                n64.mem.vi_origin = v & 0x00ffffff;
                break;
            }

            case VI_WIDTH_REG:
            {
                n64.mem.vi_width = v & 0xfff;

                // how do we get the res from this
                const auto x_old = n64.rdp.screen_x; 
                const auto y_old = n64.rdp.screen_y;

                // do we assume 4:3?
                change_res(n64.rdp,n64.mem.vi_width,(n64.mem.vi_width / 4) * 3);

                n64.size_change = (n64.rdp.screen_x != x_old || n64.rdp.screen_y != y_old);
                break;
            }

            case VI_INTR_REG:
            {
                n64.mem.vi_intr = v & 0x7ff;
                break;
            }
            
            // TODO: find out what the hell this thing does lol
            // need to start looking at how rendering works?
            case VI_BURST_REG:
            {
                n64.mem.vi_burst = v;
                break;
            }

            case VI_V_SYNC_REG:
            {
                n64.mem.vi_vsync = v & 0x7ff;
                break;
            }

            case VI_H_SYNC_REG:
            {
                n64.mem.vi_hsync = v & 0x7ff;
                break;
            }     

            // current line when written clears vi intr
            case VI_CURRENT_REG:
            {
                n64.mem.mi_intr = deset_bit(n64.mem.mi_intr,VI_INTR_BIT);
                break;
            }

            // TODO: seperate these into there fields
            case VI_LEAP_REG:
            {
                n64.mem.vi_leap = v & 0x0fffffff;
                break;
            }

            case VI_H_START_REG:
            {
                n64.mem.vi_hstart = v & ~0b000000;
                break;
            }

            case VI_V_START_REG:
            {
                n64.mem.vi_vstart = v & ~0b000000;
                break;                
            }

            case VI_V_BURST_REG:
            {
                n64.mem.vi_vburst = v & ~0b000000;
                break;
            }

            case VI_X_SCALE_REG:
            {
                n64.mem.vi_xscale = v & 0x0fffffff;
                break;
            }

            case VI_Y_SCALE_REG:
            {
                n64.mem.vi_yscale = v & 0x0fffffff;
                break;
            }

            default: unimplemented("write_mem: video interface: %8x\n",addr); break;
        }
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
                
                // dma from cart to rdram
                do_pi_dma(n64,n64.mem.pi_cart_addr,n64.mem.pi_dram_addr,n64.mem.pi_wr_len + 1);
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


    else if(addr < 0x04900000)
    {
        unimplemented("serial interface");
    }

    // UNUSED
    else if(addr < 0x05000000)
    {

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

    }



    else
    {
        std::cout << fmt::format("write_mem: unknown physical address: {:8x}:{:x}\n",addr,v);
        exit(1);
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

    addr = remap_addr(addr);

    write_physical<access_type>(n64,addr,v);
}


void do_pi_dma(N64 &n64, u32 src, u32 dst, u32 len)
{
    printf("dma from %08x to %08x len %08x\n",src,dst,len);

    // for now just do it naviely with a read and write
    // and optimise it with memcpy later
    // len aligned to 16 bit
    for(u32 i = 0; i < len / 2; i += 2)
    {
        const auto v = read_physical<u16>(n64,src + i);
        write_physical<u16>(n64,dst+i,v);
    }

    // dma is done set the intr flag
    n64.mem.mi_intr = set_bit(n64.mem.mi_intr,PI_INTR_BIT);
}



u8 read_u8(N64 &n64,u32 addr)
{
    return read_mem<u8>(n64,addr);
}

u16 read_u16(N64 &n64,u32 addr)
{
    return read_mem<u16>(n64,addr);
}

u32 read_u32(N64 &n64,u32 addr)
{
    return read_mem<u32>(n64,addr);
}

u64 read_u64(N64 &n64,u32 addr)
{
    return read_mem<u64>(n64,addr);
}


void write_u8(N64 &n64,u32 addr,u8 v)
{
    write_mem<u8>(n64,addr,v);
}

void write_u16(N64 &n64,u32 addr,u16 v)
{
    write_mem<u16>(n64,addr,v);
}

void write_u32(N64 &n64,u32 addr,u32 v)
{
    write_mem<u32>(n64,addr,v);
}

void write_u64(N64 &n64,u32 addr,u64 v)
{
    write_mem<u64>(n64,addr,v);
}



}