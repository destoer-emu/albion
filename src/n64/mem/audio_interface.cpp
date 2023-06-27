namespace nintendo64
{

void audio_event(N64& n64)
{
    auto& ai = n64.mem.ai;

    if(ai.enabled)
    {
        // dont think this is the right value but roll with it for now
        const auto event = n64.scheduler.create_event(ai.dac_rate,n64_event::ai_dma);
        n64.scheduler.insert(event,false);
    }     

    set_mi_interrupt(n64,AI_INTR_BIT);    
}

void write_ai(N64& n64, u64 addr ,u32 v)
{
    auto& ai = n64.mem.ai;

    switch(addr)
    {
        // writes clear interrupt
        case AI_STATUS:
        {
            deset_mi_interrupt(n64,AI_INTR_BIT);
            break;
        }

        case AI_CONTROL:
        {
            ai.enabled = is_set(v,0);
            if(ai.enabled)
            {
                audio_event(n64);
            }
            break;
        } 

        case AI_DACRATE:
        {
            ai.dac_rate = (v & 0b1111'1111'1111'11) + 1;
            break; 
        }

        case AI_BITRATE:
        {
            ai.bit_rate = (v & 0b1111) + 1;
            break;
        }

        case AI_DRAM_ADDR:
        {
            ai.dram_addr = v & 0x00ff'ffff;
            break;
        }

        case AI_LENGTH:
        {
            ai.length = v & 0b11'1111'1111'1111'1111;
            break;
        }

        default:
        {
            unimplemented("ai write: %x\n",addr);
            break;
        }
    }
}

u32 read_ai(N64& n64, u64 addr)
{
    auto& ai = n64.mem.ai;

    switch(addr)
    {

        case AI_STATUS:
        {
            return (ai.full << 0) | (ai.enabled << 25) | (ai.busy << 30) | (ai.full << 31); 
        }

        default:
        {
            unimplemented("ai read: %x\n",addr);
            break;
        }
    }
}
}