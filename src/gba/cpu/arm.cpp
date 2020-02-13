#include <gba/arm.h>


// register names
const char *user_regs_names[16] = 
{
    "r0","r1","r2","r3","r4","r5","r6","r7","r8",
    "r9","r10","r11","r12","sp","lr","pc"
};


const char *fiq_banked_names[5] = 
{
    "r8_fiq","r9_fiq","r10_fiq",
    "r11_fiq", "r12_fiq"
};

const char *hi_banked_names[5][2] =
{
    {"r13_fiq","r14_fiq"},
    {"r13_svc","r14_svc"},
    {"r13_abt","r14_abt"},
    {"r13_irq","r14_irq"},
    {"r13_und","r14_und"},
};

const char *status_banked_names[5] =
{
    "spsr_fiq","spsr_svc","spsr_abt",
    "spsr_irq","spsr_und",
};

const char *mode_names[7] =
{
    "FIQ","SUPERVISOR","ABORT",
    "IRQ", "UNDEFINED", "USER", "SYSTEM"
};