#include <gba/dma.h>
#include <gba/memory.h>
#include <gba/cpu.h>

namespace gameboyadvance
{

void Dma::init(Mem *mem, Cpu *cpu)
{
    this->mem = mem;
    this->cpu = cpu;
    memset(src,0,sizeof(src));
    memset(dst,0,sizeof(dst));
    memset(count,0,sizeof(count));
    memset(control,0,sizeof(control));
    memset(count_shadow,0,sizeof(count_shadow));
}

void Dma::write_src(int reg_num, int idx, uint8_t v)
{
    auto &s = src[reg_num];
    switch(idx)
    {
        case 0: s = (s & 0x0fffff00) | v;
        case 1: s = (s & 0x0fff00ff) | (8 << v);
        case 2: s = (s & 0x0f00ffff) | (16 << v);
        case 3: s = (s & 0x00ffffff) | (24 << (v & 0xf));
    }
}

void Dma::write_dst(int reg_num, int idx, uint8_t v)
{
    auto &d = dst[reg_num];
    switch(idx)
    {
        case 0: d = (d & 0x0fffff00) | v;
        case 1: d = (d & 0x0fff00ff) | (8 << v);
        case 2: d = (d & 0x0f00ffff) | (16 << v);
        case 3: d = (d & 0x00ffffff) | (24 << (v & 0xf));
    }
}

void Dma::write_count(int reg_num, int idx, uint8_t v)
{
    auto &c = dst[reg_num];
    switch(idx)
    {
        case 0: c = (c & 0xff00) | v;
        case 1: c = (c & 0x00ff) | v << 8;
    }
}


// start here
void Dma::write_control(int reg_num, int idx, uint8_t v)
{
    auto &c = control[reg_num];
    switch(idx)
    {
        case 0:
        {
            c.dst_cnt = (v >> 5) & 3;
            c.src_cnt = (c.src_cnt & ~1)  | (v >> 7);
            break;
        }

        case 1:
        {
            c.src_cnt = (c.src_cnt & ~2)  | (1 << (v & 1));
            c.dma_repeat = is_set(v,1);
            c.is_word = is_set(v,2);
            c.drq = is_set(v,3);
            c.start_time = static_cast<dma_type>((v >> 4) & 0x3);
            c.irq = is_set(v,6);
            c.enable = is_set(v,7);
            if(c.enable)
            {
                handle_dma(dma_type::immediate);
            }
            break;
        }
    }
}

uint8_t Dma::read_cnt(int reg_num,int idx) const
{
    const auto &c = control[reg_num];
    switch(idx)
    {
        case 0:
        {
            return (c.dst_cnt << 5) | ((c.src_cnt & 1) << 7);
        }

        case 1:
        {
            return ((c.src_cnt & 2) >> 1) | (c.dma_repeat << 1) | (c.is_word << 2) |
                (c.drq << 3) | (static_cast<int>(c.start_time)) << 4 | (c.irq << 6) | (c.enable << 7);
        }
    }

    return 0;
}

// check if for each dma if any of the start timing conds have been met
// should store all the dma information in struct so its nice to access
// also find out when dmas are actually processed?

// ignore dma till after armwrestler we are probably gonna redo the api for the most part anyways
void Dma::handle_dma(dma_type req_type, int special_dma)
{
    if(dma_in_progress) 
    { 
        return; 
    }


    for(int i = 0; i < 4; i++)
    {
        const auto &c = control[i];

        if(c.enable)
        {
            bool is_triggered = false;

            if(c.start_time == dma_type::special)
            {
                if(i == special_dma)
                {
                    is_triggered = true;
                }
            }

            else
            {
                is_triggered = true;
            }

            if(is_triggered)
            {
                if(c.dma_repeat)
                {
                    count_shadow[i] = count[i];
                }


                // if over max count or zero load with max length
                if(count_shadow[i] > max_count[i] || count_shadow == 0)
                {
                    count_shadow[i] = max_count[i];
                }

                do_dma(i,req_type);
            }
        }
    }
}


// what kind of side affects can we have here!?
// need to handle an n of 0 above in the calle
// also need to handle repeats!

// this needs to know the dma number aswell as the type of dma
// <-- how does gamepak dma work
// this seriously needs a refeactor
void Dma::do_dma(int reg_num,dma_type req_type)
{
    auto &c = control[reg_num];


    if(c.drq)
    {
        throw std::runtime_error("[unimplemented] gamepak dma!");
    }

    dma_in_progress = true;

    const uint32_t size = c.is_word? ARM_WORD_SIZE : ARM_HALF_SIZE;



    if(c.is_word)
    {
        for(size_t i = 0; i < count_shadow[i]; i++)
        {
            uint32_t offset = i * size;

            uint32_t v = mem->read_memt<uint32_t>(src[reg_num]+offset);
            mem->write_memt<uint32_t>(dst[reg_num]+offset,v);
            
        }
    }

    else
    {
        for(size_t i = 0; i < count_shadow[i]; i++)
        {
            uint32_t offset = i * size;

            uint16_t v = mem->read_memt<uint16_t>(src[reg_num]+offset);
            mem->write_memt<uint16_t>(dst[reg_num]+offset,v);
        }
    }

    static constexpr interrupt dma_interrupt[4] = {interrupt::dma0,interrupt::dma1,interrupt::dma2,interrupt::dma3}; 
    if(c.irq) // do irq on finish
    {
        cpu->request_interrupt(dma_interrupt[reg_num]);
    }


    if(!c.dma_repeat || req_type == dma_type::immediate || c.drq ) // dma does not repeat
    {
        c.enable = false;
    }


    switch(c.src_cnt)
    {
        case 0: // increment
        {
            src[reg_num] += count_shadow[reg_num]  * size;
            break;
        }

        case 1: // decrement
        {
            src[reg_num] -= count_shadow[reg_num] * size;
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


    switch(c.dst_cnt)
    {
        case 0: // increment
        {
            dst[reg_num] += count_shadow[reg_num] * size;
            break;
        }

        case 1: // decrement
        {
            dst[reg_num] -= count_shadow[reg_num] * size;
            break;
        }

        case 2: // fixed (do nothing)
        {
            break;
        }

        // TODO handle dst reloads (currently we dont use copys)
        case 3: // incremnt + reload
        {

            break;
        }
    }

    dma_in_progress = false;
}

};