#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

// print the prefix and then the handler name
// and print the table names
// then we can just jam them out to a file
// we will add more later
// we should extern the definitons
// so we can just dump them in one big file
void gen_mips_lut(const char *prefix)
{
    for(int i = 0; i < 32; i++)
    {

    }
}

int main()
{
    // gen the disasembly table
    gen_mips_lut("instr_")

    // gen the instruction table
    gen_mips_lut("disass_");
}