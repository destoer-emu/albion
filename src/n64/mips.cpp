namespace nintendo64
{
    
    const char *r[32]
    {
        "r0", // hardwired to zero
        "at", // assembler tempoary
        "v0", // return value or expression eval
        "v1",
        "a0", // a (argument regs)
        "a1",
        "a2",
        "a3",
        "t0",
        "t1", // t (tempoary registers not preserved across calls)
        "t2",
        "t3",
        "t4",
        "t5",
        "t6",
        "t7",
        "s0",
        "s1", // s (saved) (preserved across calls)
        "s2",
        "s3",
        "s4",
        "s5",
        "s6",
        "s7",
        "t8",
        "t9",
        "k0", // k ( kernel reserved)
        "k1",
        "gp", // global pointer (points to variable space)
        "sp", // stack pointer 
        "fp", // frame pointer
        "ra", // return address 
    };

}