#include <gba/gba.h>

namespace gameboyadvance
{

Dma::Dma(GBA &gba) : mem(gba.mem), cpu(gba.cpu)
{

}

void Dma::init()
{
    dma_in_progress = false;

    for(auto &x: dma_regs)
    {
        x.init();
    }
}


DmaReg::DmaReg()
{
    init();
}

void DmaReg::init()
{
    src = 0;
    dst = 0;
    word_count = 0;


    src_shadow = 0;
    dst_shadow = 0;
    word_count_shadow = 0;

    dst_cnt = 0;
    src_cnt = 0;
    dma_repeat = false;
    is_word = false;
    drq = false;
    start_time = dma_type::immediate;
    transfer_type = 0;
    irq = false;
    enable = false;
}


void Dma::write_source(int reg_num,int idx, uint8_t v)
{
    auto &r = dma_regs[reg_num];

    switch(idx)
    {
        case 0: r.src = (r.src & 0x0fffff00) | v; break;
        case 1: r.src = (r.src & 0x0fff00ff) | (v << 8); break;
        case 2: r.src = (r.src & 0x0f00ffff) | (v << 16); break;
        case 3: r.src = (r.src & 0x00ffffff) | ((v & 0xf) << 24); break;            
    }
}

void Dma::write_dest(int reg_num, int idx, uint8_t v)
{
    auto &r = dma_regs[reg_num];

    switch(idx)
    {
        case 0: r.dst = (r.dst & 0x0fffff00) | v; break;
        case 1: r.dst = (r.dst & 0x0fff00ff) | (v << 8); break;
        case 2: r.dst = (r.dst & 0x0f00ffff) | (v << 16); break;
        case 3: r.dst = (r.dst & 0x00ffffff) | ((v & 0xf) << 24); break;            
    }
}

void Dma::write_count(int reg_num,int idx, uint8_t v)
{
    auto &r = dma_regs[reg_num];

    switch(idx)
    {
        case 0: r.word_count = (r.word_count & 0xff00) | v; break;
        case 1: r.word_count = (r.word_count & 0x00ff) | v << 8; break;
    }    
}

uint8_t Dma::read_control(int reg_num,int idx)
{
    auto &r = dma_regs[reg_num];

    switch(idx)
    {
        case 0:
        {
            return (r.dst_cnt << 5) | ((r.src_cnt & 1) << 7);
        }

        case 1:
        {
            return ((r.src_cnt & 2) >> 1) | (r.dma_repeat << 1) | (r.is_word << 2) |
                (r.drq << 3) | (static_cast<int>(r.start_time)) << 4 | (r.irq << 6) | (r.enable << 7);
        }
    }

    return 0;
}

void Dma::write_control(int reg_num,int idx, uint8_t v)
{
    auto &r = dma_regs[reg_num];

    switch(idx)
    {
        case 0:
        {
            r.dst_cnt = (v >> 5) & 3;
            r.src_cnt = (r.src_cnt & ~1)  | (v >> 7);
            break;
        }

        case 1:
        {
            r.src_cnt = (r.src_cnt & ~2)  | ((v & 1) << 1);
            r.dma_repeat = is_set(v,1);
            r.is_word = is_set(v,2);
            r.drq = is_set(v,3);
            r.transfer_type = (v >> 4) & 0x3;


            switch(r.transfer_type)
            {

                case 0: r.start_time = dma_type::immediate; break;
                case 1: r.start_time = dma_type::vblank; break;
                case 2: r.start_time = dma_type::hblank; break;

                case 3: // special
                {
                    switch(reg_num)
                    {
                        // ???
                        case 0: throw std::runtime_error("special dma for dma0"); break;
                        case 1: r.start_time = dma_type::sound; break;
                        case 2: r.start_time = dma_type::sound; break;
                        case 3: r.start_time = dma_type::video_capture; break;
                    }
                }
            }

            r.irq = is_set(v,6);
            bool old = r.enable;
            r.enable = is_set(v,7);

            if(!old && r.enable)
            {
                // enabled reload all the shadows
                r.src_shadow = r.src;
                r.dst_shadow = r.dst;

                printf("reloaded to %x:%08x:%08x\n",reg_num,r.src_shadow,r.dst_shadow);

                // if zero reload with max length
                r.word_count_shadow = (r.word_count == 0 )? max_count[reg_num] : r.word_count;
                if(r.start_time == dma_type::immediate)
                {
                    handle_dma(dma_type::immediate);
                }
            }
            break;
        }
    }
}


// do we check dmas constatnly or can we get away with only requesting ones
// of a specific type?
void Dma::handle_dma(dma_type req_type)
{
    if(dma_in_progress)
    {
        return;
    }

    for(int i = 0; i < 4; i++)
    {
        auto &r = dma_regs[i];


        if(r.start_time == req_type && r.enable)
        {
            do_dma(i,req_type); 
        }   
    }
}


void Dma::do_dma(int reg_num, dma_type req_type)
{
    auto &r = dma_regs[reg_num];

    if(r.drq)
    {
        puts("unimplemented drq dma");
        exit(1);
    }


    const int size = r.is_word? ARM_WORD_SIZE : ARM_HALF_SIZE;


    
    switch(req_type)
    {
        // sound dma transfer 4 arm words
        // triggered by timer overflow
        // do we actually have to check the dst is the sound dma regs
        // or can software misuse it?
        // or is it determined by soundcnt_h?
        // implement soundcnt_h and the dma fires and we will find out :P
        case dma_type::sound:
        {
            //printf("fifo dma %x from %08x to %08x\n",reg_num,r.src_shadow,r.dst_shadow);

            for(int i = 0; i < 4; i++)
            {
                uint32_t v = mem.read_memt<uint32_t>(r.src_shadow+i*ARM_WORD_SIZE);
                mem.write_memt<uint32_t>(r.dst_shadow,v);
            }
            
            switch(r.src_cnt)
            {
                case 0: r.src_shadow += 4; break;
                case 1: r.src_shadow -= 4; break;
                case 2: break;
                case 3: throw std::runtime_error("sad mode of 3");
            }

            return;
        }


        default:
        {
            dma_in_progress = true;


            printf("dma %x from %08x to %08x\n",reg_num,r.src_shadow,r.dst_shadow);

            if(r.is_word)
            {
                for(size_t i = 0; i < r.word_count_shadow; i++)
                {
                    uint32_t offset = i * size;

                    uint32_t v = mem.read_memt<uint32_t>(r.src_shadow+offset);
                    mem.write_memt<uint32_t>(r.dst_shadow+offset,v);
                    
                }
            }

            else
            {
                for(size_t i = 0; i < r.word_count_shadow; i++)
                {
                    uint32_t offset = i * size;

                    uint16_t v = mem.read_memt<uint16_t>(r.src_shadow+offset);
                    mem.write_memt<uint16_t>(r.dst_shadow+offset,v);
                }
            }
            break;
        }
    }

    // do irq on finish
    static constexpr interrupt dma_interrupt[4] = {interrupt::dma0,interrupt::dma1,interrupt::dma2,interrupt::dma3}; 
    if(r.irq) 
    {
        cpu.request_interrupt(dma_interrupt[reg_num]);
    }


    // dma does not repeat
    if(!r.dma_repeat || req_type == dma_type::immediate || r.drq ) 
    {
        r.enable = false;
    }

    dma_in_progress = false;

    // in fifo timing mode (sound dma)
    // increments dont happen
    if(req_type != dma_type::sound)
    {
        handle_increment(reg_num);
    }
}



void Dma::handle_increment(int reg_num)
{
    auto &r = dma_regs[reg_num];

    const int size = r.is_word? ARM_WORD_SIZE : ARM_HALF_SIZE;

    switch(r.src_cnt)
    {
        case 0: // increment
        {
            r.src_shadow += r.word_count_shadow  * size;
            break;
        }

        case 1: // decrement
        {
            r.src_shadow -= r.word_count_shadow * size;
            break;
        }

        case 2: // fixed (do nothing)
        {
            break;
        }

        case 3: // invalid
        {
            throw std::runtime_error("sad mode of 3!");
        }
    }


    switch(r.dst_cnt)
    {
        case 0: // increment
        {
            r.dst_shadow += r.word_count_shadow * size;
            break;
        }

        case 1: // decrement
        {
            r.dst_shadow -= r.word_count_shadow * size;
            break;
        }

        case 2: // fixed (do nothing)
        {
            break;
        }

        
        case 3: // incremnt + reload
        {
            r.dst_shadow = r.dst + r.word_count_shadow;
            break;
        }
    }
}

};