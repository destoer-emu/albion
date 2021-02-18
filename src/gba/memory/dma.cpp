#include <gba/gba.h>

namespace gameboyadvance
{


// TODO double check dma priority

Dma::Dma(GBA &gba) : mem(gba.mem), cpu(gba.cpu) , debug(gba.debug)
{

}

void Dma::init()
{
    for(auto &x: dma_regs)
    {
        x.init();
    }

    req_list.clear();
    active_dma = -1;
}


DmaReg::DmaReg(int reg) : max_count(max_counts[reg]), dma_interrupt(dma_interrupts[reg])
{
    assert(reg >= 0 && reg <= 3);
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
    interrupted = false;
}

// theres def a faster way to handle this masking but we wont worry about it for now


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
/*  todo verify?
    // probably a cleaner way to handle the 27 bit limit 
    if(reg_num != 3)
    {
        r.src = deset_bit(r.src,27);
    }
*/
    //printf("dma source written! %08x:%08x\n",r.src,cpu.get_pc());
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

    // probably a cleaner way to handle the 27 bit limit 
    if(reg_num != 3)
    {
        r.dst = deset_bit(r.dst,27);
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
    
    // enforce the word count limit
    r.word_count &= r.max_count - 1;    
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
                        case 0: r.start_time = dma_type::invalid; break;
                        case 1:
                        case 2: 
                        {
                            // need to verifiy hardware actually cares about the dst
                            if(r.dst == 0x040000A0) // fifo a
                            {
                                r.start_time = dma_type::fifo_a;
                            }

                            else if(r.dst == 0x040000A4)
                            {
                                r.start_time = dma_type::fifo_b;
                            }

                            else
                            {
                                r.start_time = dma_type::invalid;
                            }
                            break;

                        }
                        case 3: r.start_time = dma_type::video_capture; break;
                    }
                    break;
                }
            }

            r.irq = is_set(v,6);
            const bool old = r.enable;
            r.enable = is_set(v,7);

            if(!old && r.enable)
            {
                // enabled reload all the shadows
                r.src_shadow = r.src;
                r.dst_shadow = r.dst;

                //printf("[%08x]reloaded to %x:%08x:%08x:%d:%d:%x\n",cpu.get_pc(),reg_num,r.src_shadow,r.dst_shadow,r.dst_cnt,r.src_cnt,r.word_count);

                if(r.start_time == dma_type::immediate)
                {
                    handle_dma(dma_type::immediate);
                }
            }

            // if its been turned off in the middle
            // of the xfer we need to break out early
            if(reg_num == active_dma && !r.enable)
            {
                r.interrupted = true;
            }

            break;
        }
    }
}
// todo emulate 2 cycle startup from cpu to dma...  
void Dma::handle_dma(dma_type req_type)
{
    // a active dma will ofhold all others until it is done
    // even if it has a lower priority
    if(active_dma != -1)
    {
        // this needs to be made faster
        // we need a quick way to lookup
        // all of a given req type
        // and find the one with the higest priority


        // check if a dma needs to overtake another
        for(int i = 0; i < 4; i++)
        { 
            const auto &r = dma_regs[i];


            if(r.enable && r.start_time == req_type)
            {
                // this is at a higher priority
                if(active_dma > i)
                {
                    dma_regs[active_dma].interrupted = true;
                }
            }
        }
    

        // push this to a list of types that need to get checked
        req_list.push_back(req_type);
        return;
    }

    check_dma(req_type);

    // dmas could of become queued during the last one
    // so we need to recheck them until there no more to
    // service
    while(req_list.size())
    {
        const auto req = req_list.back();
        req_list.pop_back();
        check_dma(req);
    }
}


void Dma::check_dma(dma_type req_type)
{
    // higher priority dmas can hijack lower ones during transfer
    for(int i = 0; i < 4; i++)
    {
        const auto &r = dma_regs[i];


        if(r.enable && r.start_time == req_type)
        {
            active_dma = i;
            do_dma(i,req_type); 
            active_dma = -1;
        }   
    }    
}


void Dma::turn_off_video_capture()
{
    if(dma_regs[3].start_time == dma_type::video_capture)
    {
        dma_regs[3].enable = false;
    }
}

bool Dma::do_fast_dma(int reg_num)
{
    auto &r = dma_regs[reg_num];


    // okay for now lets just handle both src & dst incrementing
    if(r.src_cnt != 0 || r.dst_cnt != 0)
    {
        return false;
    }

    bool success = false;

    if(r.is_word)
    {
        success = mem.fast_memcpy<uint32_t>(r.dst_shadow,r.src_shadow,r.word_count_shadow);        
    }

    else
    {
        success = mem.fast_memcpy<uint16_t>(r.dst_shadow,r.src_shadow,r.word_count_shadow); 
    }

    if(success)
    {
        // increment + reload is forbidden dont use it
        if(r.src_cnt != 3)
        {
            r.src_shadow += addr_increment_table[r.is_word][r.src_cnt] * r.word_count_shadow;
        }

        r.dst_shadow += addr_increment_table[r.is_word][r.dst_cnt] * r.word_count_shadow;

        // todo haddle the dma timing
        //const auto src_reg = memory_region_table[(r.src_shadow >> 24) & 0xf];
        //cycle_tick(r.word_count_shadow * )
        //const auto dst_reg = memory_region_table[(r.dst_shadow >> 24) & 0xf];
        //puts("hi");
        // TODO FIX THSI APPROXMIATION
        //cpu.cycle_tick(r.word_count_shadow*2);
    }

    return success;
}

void Dma::do_dma(int reg_num, dma_type req_type)
{
    auto &r = dma_regs[reg_num];


    // reload word count
    r.word_count_shadow = (r.word_count == 0 )? r.max_count: r.word_count;

    // if in dst_cnt is in mode 3
    // reload the dst
    if(r.dst_cnt == 3)
    {
        r.dst_shadow = r.dst;
    }

    
    r.interrupted = false;


    switch(req_type)
    {
        // sound dma transfer 4 arm words
        // triggered by timer overflow
        case dma_type::fifo_b:
        case dma_type::fifo_a:
        {
            //printf("fifo dma %x from %08x to %08x\n",reg_num,r.src_shadow,r.dst_shadow);


            // need to rework our memory model to handle
            // the n & s cycles implictly at some point
            // dma takes 2N + 2(n-1)s +xI
            for(size_t i = 0; i < 4; i++)
            {
                const auto v = mem.read_memt<uint32_t>(r.src_shadow);
                mem.write_memt<uint32_t>(r.dst_shadow,v);
                
                // increment + reload is forbidden dont use it
                if(r.src_cnt != 3)
                {
                    // allways in word mode here
                    r.src_shadow += addr_increment_table[r.is_word][r.src_cnt];
                }

                // dst is not incremented when doing fifo dma
            }
            break;
        }


        default:
        {
            //write_log(debug,"dma {:x} from {:08x} to {:08x}\n",reg_num,r.src_shadow,r.dst_shadow);
            //std::cout << fmt::format("dma {:x} from {:08x} to {:08x}\n",reg_num,r.src_shadow,r.dst_shadow);
            // TODO how does internal cycles work for this?

            // todo check for interrupts when we actually handle dma priority

            // cannot easily use a memcpy
            if(!do_fast_dma(reg_num))
            {
                if(r.is_word)
                {
                    for(size_t i = 0; i < r.word_count_shadow; i++)
                    {
                        const auto v = mem.read_memt<uint32_t>(r.src_shadow);
                        mem.write_memt<uint32_t>(r.dst_shadow,v);
                        handle_increment(reg_num);  
                    }
                }

                else
                {
                    for(size_t i = 0; i < r.word_count_shadow; i++)
                    {
                        const auto v = mem.read_memt<uint16_t>(r.src_shadow);
                        mem.write_memt<uint16_t>(r.dst_shadow,v);
                        handle_increment(reg_num);
                    }
                }
            }
            break;
        }
    }

    // do irq on finish
    if(r.irq) 
    {
        cpu.request_interrupt(r.dma_interrupt);
    }


    // dma does not repeat
    if(!r.dma_repeat || req_type == dma_type::immediate || r.drq ) 
    {
        r.enable = false;
    }
}


void Dma::handle_increment(int reg_num)
{
    auto &r = dma_regs[reg_num];

    
    // increment + reload is forbidden dont use it
    if(r.src_cnt != 3)
    {
        r.src_shadow += addr_increment_table[r.is_word][r.src_cnt];
    }

    r.dst_shadow += addr_increment_table[r.is_word][r.dst_cnt];
}

};