#include<gb/gb.h>


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
    auto cycles = event_list.peek().end - timestamp;

    // make sure we are on a 4 cycle boundary
    cycles = (cycles + 3) & ~0x3; 

    cpu.cycle_tick_t(cycles);
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
            apu.c1.tick_period(cycles_to_tick >> is_double());
            break;
        }

        case gameboy_event::c2_period_elapse:
        {
            apu.c2.tick_period(cycles_to_tick >> is_double());
            break;
        }

        case gameboy_event::c3_period_elapse:
        {
            apu.c3.tick_period(cycles_to_tick >> is_double());
            break;
        }

        case gameboy_event::c4_period_elapse:
        {
            apu.c4.tick_period(cycles_to_tick >> is_double());
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

        case gameboy_event::ppu:
        {
            //printf("service: %d:%d:%d\n",node.start,node.end,timestamp);
            ppu.update_graphics(cycles_to_tick >> is_double());
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