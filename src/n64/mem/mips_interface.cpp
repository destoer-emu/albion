#include <n64/n64.h>

namespace nintendo64
{

void mi_intr(N64& n64);
void deset_intr_cop0(N64& n64, u32 bit);

void check_mi_interrupts(N64& n64)
{
    auto& mi = n64.mem.mi;

    //print("mi mask {:b} : {:b}\n",mi.mask,mi.intr);

    if(mi.mask & mi.intr)
    {
        mi_intr(n64);
    }

    else
    {
        deset_intr_cop0(n64,MI_BIT);
    }
}


b32 mi_intr_set(N64& n64, u32 bit)
{
    auto& mi = n64.mem.mi;
    return is_set(mi.intr,bit);
}

void set_mi_interrupt(N64& n64, u32 bit)
{
    auto& mi = n64.mem.mi;
    mi.intr = set_bit(mi.intr,bit);
    check_mi_interrupts(n64);
}

void deset_mi_interrupt(N64& n64, u32 bit)
{
    auto& mi = n64.mem.mi;
    mi.intr = deset_bit(mi.intr,bit);
    check_mi_interrupts(n64);
}

void write_mi(N64& n64, u64 addr, u32 v)
{
    auto& mi = n64.mem.mi;

    switch(addr)
    {  
        case MI_MODE: 
        {
            mi.mode = v; 
            if(is_set(v,11)) // bit 11 wrote clear DP interrupt bit 
            {
                deset_mi_interrupt(n64,DP_INTR_BIT);
            }
            break;
        }

        // read only?
        case MI_INTERRUPT:
        {
            break;
        }

        case MI_INTR_MASK:
        {
            mi.mask = deset_bitset_if_set(mi.mask,v,0,SP_INTR_BIT);
            mi.mask = set_bitset_if_set(mi.mask,v,1,SP_INTR_BIT);

            mi.mask = deset_bitset_if_set(mi.mask,v,2,SI_INTR_BIT);
            mi.mask = set_bitset_if_set(mi.mask,v,3,SI_INTR_BIT);

            mi.mask = deset_bitset_if_set(mi.mask,v,4,AI_INTR_BIT);
            mi.mask = set_bitset_if_set(mi.mask,v,5,AI_INTR_BIT);

            mi.mask = deset_bitset_if_set(mi.mask,v,6,VI_INTR_BIT);
            mi.mask = set_bitset_if_set(mi.mask,v,7,VI_INTR_BIT);

            mi.mask = deset_bitset_if_set(mi.mask,v,8,PI_INTR_BIT);
            mi.mask = set_bitset_if_set(mi.mask,v,9,PI_INTR_BIT);

            mi.mask = deset_bitset_if_set(mi.mask,v,10,DP_INTR_BIT);
            mi.mask = set_bitset_if_set(mi.mask,v,11,DP_INTR_BIT);
            check_mi_interrupts(n64);
            break;
        }

        default: unimplemented("write_mem: mips interface : %8x\n",addr); break;
    }
}

u32 read_mi(N64& n64, u64 addr)
{
    auto& mi = n64.mem.mi;

    switch(addr)
    {
        case MI_VERSION: return 0x02020102;

        case MI_INTR_MASK: return mi.mask;

        case MI_INTERRUPT: return mi.intr;

        default:
        {
            unimplemented("read_mem: mips interface: %8x\n",addr);
            return 0;
        }
    }    
}


}