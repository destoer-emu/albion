namespace nintendo64
{

void insert_audio_event(N64& n64)
{
    auto& ai = n64.mem.ai;
    UNUSED(ai);

    // dont think this is the right value but roll with it for now
    const auto event = n64.scheduler.create_event(ai.freq,n64_event::ai_dma);
    n64.scheduler.insert(event,false);    
}

void do_ai_dma(N64& n64)
{
    UNUSED(n64);
}

void audio_event(N64& n64)
{
    auto& ai = n64.mem.ai;

    // dma over
    ai.busy = false;

    // interrupt as transfer is done
    set_mi_interrupt(n64,AI_INTR_BIT);  

    // handle pending transfer
    if(ai.full && ai.enabled)
    {
        ai.full = false;
        ai.busy = true;

        // we just have it do this instantly
        do_ai_dma(n64);

        insert_audio_event(n64);
    }
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
            break;
        } 

        case AI_DACRATE:
        {
            ai.dac_rate = (v & 0b1111'1111'1111'11);
            ai.freq = VIDEO_CLOCK / (ai.dac_rate + 1);
            printf("freq : %d\n",ai.freq);
            ai.freq = 44100;
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
            if(ai.enabled)
            {
                // initial dma
                if(!ai.busy)
                {
                    // interrupt for intial transfer
                    set_mi_interrupt(n64,AI_INTR_BIT);

                    ai.busy = true;

                    // we just have it do this instantly
                    do_ai_dma(n64);

                    // setup event for transfer end!
                    // TODO: calculated freq is botched
                    insert_audio_event(n64);
                }

                // we are busy see if we can setup a pending dma
                else if(!ai.full)
                {
                    // pending transfer
                    ai.full = true;
                }
            }
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