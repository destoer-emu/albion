
namespace nintendo64
{

// NOTE: all intr handling goes here

static constexpr u32 COUNT_BIT = 7;
static constexpr u32 MI_BIT = 2;


// see page 151 psuedo code of manual
void standard_exception(N64& n64, u32 code)
{
    // TODO: i think we need to run tests for this LOL
    auto& cop0 = n64.cpu.cop0;
    auto& status = cop0.status;
    auto& cause = cop0.cause;

    if(!status.exl)
    {
        status.exl = true;
        cause.exception_code = code;

        // interrupt happens at start of instr
        if(code == beyond_all_repair::INTERRUPT)
        {
            if(!in_delay_slot(n64.cpu))
            {
                cop0.epc = n64.cpu.pc;
                cause.branch_delay = false;
            }

            else
            {
                cop0.epc = n64.cpu.pc - MIPS_INSTR_SIZE;
                cause.branch_delay = true;
            }
        }

        else
        {
            // pc will be ahead + 4
            //assert(false);

            if(!in_delay_slot(n64.cpu))
            {
                cop0.epc = n64.cpu.pc - MIPS_INSTR_SIZE;
                cause.branch_delay = false;
            }

            else
            {
                cop0.epc = n64.cpu.pc - (MIPS_INSTR_SIZE * 2);
                cause.branch_delay = true;
            }
        }

        // bev bit is set
        if(is_set(status.ds,6))
        {
            unimplemented("bev interrupt");
        }

        else
        {
            u32 vector = 0;

            switch(code)
            {
                // how do we know what kind of tlb refill it is lol
                case TLBL: vector = 0x0; assert(false); break;
                case TLBS: vector = 0x080; assert(false); break;

                default: vector = 0x180; break;
            }

            const u32 target = 0xFFFF'FFFF'8000'0000 + vector; 

            // TODO: do we have to wait to write this inside the step func?
            write_pc(n64,target);
            skip_instr(n64.cpu);
        }
    }

    // double fault
    else
    {
        assert(false);
    }
}

void coprocesor_unusable(N64& n64, u32 number)
{
    // set coprocessor number, then its just a standard exception
    // with the cop exception code?
    auto& cause = n64.cpu.cop0.cause;
    cause.coprocessor_error = number;
    standard_exception(n64,COP_UNUSABLE);
}

void error_exception(N64& n64, u32 code)
{
    assert(false);
    UNUSED(n64); UNUSED(code); 
}

void check_interrupts(N64 &n64)
{
    auto& cop0 = n64.cpu.cop0;
    auto& status = cop0.status;
    auto& cause = cop0.cause;


    //printf("boop : %x : %d : %d : %d\n",cause.pending,status.ie,status.erl,status.exl);

    // enable off or execption being serviced
    // no interrupts on
    if(!status.ie || status.erl || status.exl)
    {
        return;
    }

    // mask enabled with pending interrupt
    const b32 pending = (status.im & cause.pending) != 0;

    if(pending)
    {
        n64.cpu.interrupt = true;
    }
}


b32 cop0_usable(N64& n64)
{
    auto& status = n64.cpu.cop0.status;

    // coprocesor unusable if disabled and not in kernel mode
    if(!status.cu0 && (status.ksu != KERNEL_MODE))
    {
        coprocesor_unusable(n64,0);
        return false;
    }

    return true;
}


void insert_count_event(N64 &n64)
{
    u64 cycles;

    auto& cop0 = n64.cpu.cop0;

    const u32 count = cop0.count;
    const u32 compare = cop0.compare;

    
    if(count < compare)
    {
        cycles = (compare - count);
    }

    else
    {
        cycles = (0xffffffff - (compare - count)); 
    }

    const auto event = n64.scheduler.create_event(cycles * 2,n64_event::count);
    n64.scheduler.insert(event,false); 
}


void set_intr_cop0(N64& n64, u32 bit)
{
    auto& cause = n64.cpu.cop0.cause;

    cause.pending = set_bit(cause.pending,bit);
    check_interrupts(n64);
}

void deset_intr_cop0(N64& n64, u32 bit)
{
    auto& cause = n64.cpu.cop0.cause;

    cause.pending = deset_bit(cause.pending,bit);
}

// NOTE: do this relative to your storage,
// start tomorrow you are too tired
void count_intr(N64 &n64)
{
    set_intr_cop0(n64,COUNT_BIT);
}

void check_count_intr(N64& n64)
{
    auto& cop0 = n64.cpu.cop0;

    printf("cmp : %d : %d\n",cop0.count,cop0.compare);

    // account for our shoddy timing..
    if(beyond_all_repair::abs(cop0.count - cop0.compare) <= 25)
    {
        cop0.count = cop0.compare;
        count_intr(n64);        
    }
}

void count_event(N64& n64, u32 cycles)
{
    auto& cop0 = n64.cpu.cop0;
    cop0.count += cycles >> 1;

    check_count_intr(n64);

    insert_count_event(n64);
}

void mi_intr(N64& n64)
{
    set_intr_cop0(n64,MI_BIT);
}

void write_entry_lo(EntryLo& entry_lo, u32 v)
{
    entry_lo.pfn = (v >> 6) & 0x00ff'ffff;
    entry_lo.c = (v >> 3) & 0b11;
    entry_lo.d = is_set(v,2);
    entry_lo.v = is_set(v,1);
    entry_lo.g = is_set(v,0);
}

u32 read_entry_lo(EntryLo& entry_lo)
{
    return (entry_lo.g << 0) | (entry_lo.v << 1) | (entry_lo.d << 2) | 
        (entry_lo.c << 3) | (entry_lo.pfn << 6); 
}


void write_cop0(N64 &n64, u64 v, u32 reg)
{
    auto &cpu = n64.cpu;
    auto &cop0 = cpu.cop0;

    switch(reg)
    {
        // cycle counter (incremented every other cycle)
        case COUNT:
        {
            puts("wrote count");
            n64.scheduler.remove(n64_event::count,false);
            cop0.count = v;
            insert_count_event(n64);
            break;
        }

        // when this is == count trigger in interrupt
        case COMPARE:
        {
            n64.scheduler.remove(n64_event::count);
            cop0.compare = v;
            insert_count_event(n64);

            // ip7 in cause is reset when this is written
            deset_intr_cop0(n64,COUNT_BIT);
            break;
        }
        
        case TAGLO: // for cache, ignore for now
        {
            cop0.tag_lo = v;
            break;
        }

        case TAGHI: // for cache, ignore for now
        {
            cop0.tag_hi = v;
            break;
        }

        // various cpu settings
        case STATUS:
        {
            auto& status = cop0.status;

            status.ie = is_set(v,0);
            status.exl = is_set(v,1);
            status.erl = is_set(v,2);
            status.ksu = (v >> 3) & 0b11;

            status.ux = is_set(v,5);
            status.sx = is_set(v,6);
            status.kx = is_set(v,7);

            status.im = (v >> 8) & 0xff;

            status.ds = (v >> 16) & 0x1ff;

            status.re = is_set(v,25);
            status.fr = is_set(v,26);
            status.rp = is_set(v,27);

            status.cu0 = is_set(v,28);
            status.cu1 = is_set(v,29);
            status.cu2 = is_set(v,30);
            status.cu3 = is_set(v,31);


            if(status.rp)
            {
                unimplemented("low power mode");
            }

            if(status.re)
            {
                unimplemented("little endian");
            }

            if((status.ux && status.ksu == 0b10) || (status.sx && status.ksu == 0b01) || (status.kx && status.ksu == 0b00))
            {
                unimplemented("64 bit addressing");
            }

            check_interrupts(n64);
            break;
        }

        // interrupt / exception info
        case CAUSE:
        {
            auto& cause = cop0.cause;

            const auto PENDING_MASK = 0b11;

            // write can only modify lower 2 bits of interrupt pending
            cause.pending = (cause.pending & ~PENDING_MASK)  | ((v >> 8) & PENDING_MASK);
            check_interrupts(n64);
            break;
        }

        case ENTRY_HI:
        {
            auto& entry_hi = cop0.entry_hi;
            entry_hi.vpn2 = (v >> 13);
            entry_hi.asid = v & 0xff; 
            break;
        }

        case ENTRY_LO_ZERO:
        {
            write_entry_lo(cop0.entry_lo_zero,v);
            break;
        }

        case ENTRY_LO_ONE:
        {
            write_entry_lo(cop0.entry_lo_one,v);
            break;
        }

        // TODO: what is this used for?
        case PRID:
        {
            cop0.prid = v;
            break;
        }

        case CONFIG:
        {
            cop0.config = v;
            break;
        }

        case INDEX:
        {
            auto& index = cop0.index;
            index.p = is_set(v,31);
            index.idx = v & 0b111'111;
            break;
        }

        case PAGE_MASK:
        {
            cop0.page_mask = (v >> 13) & 0xfff;
            break;
        }

        case EPC:
        {
            cop0.epc = v;
            break;
        }

        case ERROR_EPC:
        {
            cop0.error_epc = v;
            break;
        }

        case BAD_VADDR:
        {
            cop0.bad_vaddr = v;
            break;
        }

        // read only
        case RANDOM: break;

        default:
        {
            printf("unimplemented cop0 write: %s(%d)\n",COP0_NAMES[reg],reg);
            exit(1);
        }
    }
}

u64 read_cop0(N64& n64, u32 reg)
{
    UNUSED(n64);

    auto& cpu = n64.cpu;
    auto& cop0 = cpu.cop0;

    switch(reg)
    {
        case STATUS:
        {
            auto& status = cop0.status;

            return status.ie | (status.exl << 1) | (status.erl << 2) |
                (status.ksu << 3) | (status.ux << 5) | (status.sx << 6) |
                (status.kx << 7) | (status.im << 8) | (status.ds << 8) |
                (status.re << 25) | (status.fr << 26) | (status.rp << 27) |
                (status.cu0 << 28) | (status.cu1 << 29) | (status.cu2 << 30) | 
                (status.cu3 << 31);
        }

        case ENTRY_HI:
        {
            auto& entry_hi = cop0.entry_hi;
            return (entry_hi.asid << 0) | (entry_hi.vpn2 << 13);
        }

        case ENTRY_LO_ZERO:
        {
            return read_entry_lo(cop0.entry_lo_zero);
        }

        case ENTRY_LO_ONE:
        {
            return read_entry_lo(cop0.entry_lo_one);
        }

        case PAGE_MASK:
        {
            return (cop0.page_mask << 13);
        }

        case COUNT:
        {
            return cop0.count;
        }

        case EPC:
        {
            return cop0.epc;
        }

        case CAUSE:
        {
            auto& cause = cop0.cause;
            return (cause.exception_code << 2) | (cause.pending  << 8) | (cause.coprocessor_error << 28) | (cause.branch_delay << 31); 
        }

        case INDEX:
        {
            auto& index = cop0.index;
            return (index.idx << 0) | (index.p << 31);
        }

        case ERROR_EPC:
        {
            return cop0.error_epc;
        }

        case BAD_VADDR:
        {
            return cop0.bad_vaddr;
        }

        case CONTEXT:
        {
            auto& context = cop0.context;
            return (context.bad_vpn2 << 4) | (context.pte_base << 23);
        }

        case COMPARE:
        {
            return cop0.compare;
        }

        default:
        {
            printf("unimplemented cop0 read: %s(%d)\n",COP0_NAMES[reg],reg);
            exit(1);
        }        
    }
}

}