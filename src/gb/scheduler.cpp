#include<gb/gb.h>
#include<gb/cpu.inl>


// investiage oracle games audio cut out
// and alone in the dark
// think former is because of the remove change
// and not having insertion handle weird behavior

namespace gameboy
{

// this needs a save state impl

GameboyScheduler::GameboyScheduler(GB &gb) : cpu(gb.cpu), ppu(gb.ppu), 
    apu(gb.apu), mem(gb.mem)
{
    init();
}

// option to align cycles for things like waitloop may be required
void GameboyScheduler::skip_to_event()
{
    auto cycles = min_timestamp - timestamp;

    // make sure we are on a 4 cycle boundary
    cycles = (cycles + 3) & ~0x3; 

    cpu.cycle_tick_t(cycles);
    service_events();
} 


// better way to handle this? std::function is slow
void GameboyScheduler::service_event(const EventNode<gameboy_event> &node)
{
    // if its double speed we need to push half the cycles
    // through the function even though we delay for double
    const auto cycles_to_tick = timestamp - node.start;

    switch(node.type)
    {
        case gameboy_event::oam_dma_end:
        {
            mem.tick_dma(cycles_to_tick);
            break;
        }

        case gameboy_event::c1_period_elapse:
        {
            if(square_tick_period(apu.psg.channels[0],cycles_to_tick >> is_double()))
            {
                apu.insert_chan1_period_event();
            }
            break;
        }

        case gameboy_event::c2_period_elapse:
        {
            if(square_tick_period(apu.psg.channels[1],cycles_to_tick >> is_double()))
            {
                apu.insert_chan2_period_event();
            }
            break;
        }

        case gameboy_event::c3_period_elapse:
        {
            if(wave_tick_period(apu.psg.wave,apu.psg.channels[2],cycles_to_tick >> is_double()))
            {
                apu.insert_chan3_period_event();
            }
            break;
        }

        case gameboy_event::c4_period_elapse:
        {
            if(noise_tick_period(apu.psg.noise,apu.psg.channels[3],cycles_to_tick >> is_double()))
            {
                apu.insert_chan4_period_event();
            }
            break;
        }

        case gameboy_event::sample_push:
        {
            apu.push_samples(cycles_to_tick >> is_double());
            break;
        }

        case gameboy_event::internal_timer:
        {
            cpu.update_timers(cycles_to_tick);
            break;
        }

        case gameboy_event::timer_reload:
        {
            // just do the operation here
            // how is this affected by double speed?
            cpu.tima_reload();
            break;
        }

        case gameboy_event::ppu:
        {
            //printf("service: %d:%d:%d\n",node.start,node.end,timestamp);
            ppu.update_graphics(cycles_to_tick >> is_double());
            break;
        }

        case gameboy_event::serial:
        {
            cpu.tick_serial(cycles_to_tick);
            break;
        }

    }
}


// just because its convenient 
bool GameboyScheduler::is_double() const
{
    return cpu.get_double();
}

}