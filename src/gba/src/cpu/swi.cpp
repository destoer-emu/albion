#include <gba/gba.h>

namespace gameboyadvance
{

// bios hle (very unfinished - no games will boot under it)

void Cpu::swi(u32 function)
{
    switch(function)
    {
        case 0x1: // register ram set
        {

            if(is_set(regs[R0],0))
            {
                std::fill(mem.board_wram.begin(),mem.board_wram.end(),0);
            }

            if(is_set(regs[R0],1))
            {
                std::fill(mem.chip_wram.begin(),mem.chip_wram.end()-0x200,0);
            }

            if(is_set(regs[R0],2))
            {
                std::fill(mem.pal_ram.begin(),mem.pal_ram.end(),0);
            }

            if(is_set(regs[R0],3))
            {
                std::fill(mem.vram.begin(),mem.vram.end(),0);
            }

            if(is_set(regs[R0],4))
            {
                std::fill(mem.oam.begin(),mem.oam.end(),0);
            }
/*          clears sio regs
            if(is_set(regs[R0]),5)
            {

            }
*/
            // sound regs
            if(is_set(regs[R0],6))
            {
                for(int i = 0x04000060; i < 0x04000088; i++)
                {
                    mem.write_u8(i,0);
                }
            }

            if(is_set(regs[R0],7))
            {
                // reset all other regs
                for(int i = 0x04000000; i < 0x040003FE; i++)
                {
                    // ignore sound regs
                    if(i >= 0x04000060 && i <= 0x04000088)
                    {
                        continue;
                    }

                    mem.write_u8(i,0);
                }   
            }

            break;
        }

        // memset / memcpy
        case 0xc:
        {
            const auto src = regs[R0];
            const auto dst = regs[R1];

            const auto cnt = (regs[R2] / 4) & ((1 << 22) - 1);

            const bool memset = is_set(regs[R2],24);

            

            if(memset)
            {
                const auto v = mem.read_u32(regs[R0]);

                for(size_t i = 0; i < cnt; i++)
                {
                    mem.write_u32(dst + (i*4),v);
                }
            }

            else
            {
                if(!mem.fast_memcpy<u32>(dst,src,cnt))
                {
                    for(size_t i = 0; i < cnt; i++)
                    {
                        const auto v = mem.read_u32(src + (i * 4));
                        mem.write_u32((dst + (i * 4)),v);
                    }
                }
            }
            break;
        }


        default:
            printf("unknown swi: %x\n",function);
            exit(1);
            break;
    }
}

}