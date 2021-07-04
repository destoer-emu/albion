#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

void gen_mips_lut(const char *prefix)
{
    for(int i = 0; i < 64; i++)
    {
        printf("&%s",prefix);
        switch(i)
        {
            // lw
            case 0b100011:
            {
                printf("lw");
                break;
            }

            // addiu
            case 0b001001:
            {
                printf("addiu");
                break;
            }

            // lui:
            case 0b001111:
            {
                printf("lui");
                break;
            } 

            // cop0
            case 0b010000:
            {
                printf("cop0");
                break;
            }

            default:
            {
                printf("unknown");
                break;
            }
        }

        // end entry
        printf(",\n");
    }
}


void gen_cop0_lut(const char *prefix)
{
    for(int i = 0; i < 32; i++)
    {
        printf("&%s",prefix);
        switch(i)
        {
            // cop0
            case 0b00100:
            {
                printf("mtc0");
                break;
            }

            default:
            {
                printf("unknown_cop0");
                break;
            }
        }

        // end entry
        printf(",\n");
    }    
}



// do we really want to handle the names this way
int main()
{
    printf("#include <n64/n64.h>\n\n");

    printf("namespace nintendo64 {\n");




    printf("const INSTR_FUNC instr_lut[] = {\n");

    // gen the instr table
    gen_mips_lut("instr_");

    printf("};\n\n\n");



    printf("const DISASS_FUNC disass_lut[] = {\n");

    // gen the disass table
    gen_mips_lut("disass_");

    printf("};\n\n\n");




    // and for cop0


    printf("const INSTR_FUNC instr_cop0_lut[] = {\n");

    // gen the instr table
    gen_cop0_lut("instr_");

    printf("};\n\n\n");



    printf("const DISASS_FUNC disass_cop0_lut[] = {\n");

    // gen the disass table
    gen_cop0_lut("disass_");

    printf("};\n\n\n");




    printf("}\n");
}