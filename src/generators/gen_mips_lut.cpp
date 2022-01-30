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
            // r-format instruction
            // TODO: start here
            case 0b000000:
            {
                printf("r_fmt");
                break;
            }

            // regimm instr
            case 0b000001:
            {
                printf("regimm");
                break;
            }

            // blezl
            case 0b010110:
            {
                printf("blezl");
                break;
            }

            // slti:
            case 0b001010:
            {
                printf("slti");
                break;
            }

            // cache
            case 0b101111:
            {
                printf("cache");
                break;
            }


            // beql:
            case 0b010100:
            {
                printf("beql");
                break;
            }

            case 0b010101:
            {
                printf("bnel");
                break;
            }

            // andi
            case 0b001100:
            {
                printf("andi");
                break;
            }

            // xori
            case 0b001110:
            {
                printf("xori");
                break;
            }

            // bne
            case 0b000101:
            {
                printf("bne");
                break;
            }

            case 0b000100:
            {
                printf("beq");
                break;
            }


            case 0b110111:
            {
                printf("ld");
                break;
            }

            case 0b000111:
            {
                printf("bgtz");
                break;
            }

            case 0b0101001:
            {
                printf("sh");
                break;
            }

            case 0b111111:
            {
                printf("sd");
                break;
            }

            case 0b100000:
            {
                printf("lb");
                break;
            }

            case 0b100101:
            {
                printf("lhu");
                break;
            }

            // lw
            case 0b100011:
            {
                printf("lw");
                break;
            }

            // jal
            case 0b000011:
            {
                printf("jal");
                break;
            }

            case 0b101011:
            {
                printf("sw");
                break;
            }

            // sb
            case 0b101000:
            {
                printf("sb");
                break;
            }

            // lbu
            case 0b100100:
            {
                printf("lbu");
                break;
            }

            // addiu
            case 0b001001:
            {
                printf("addiu");
                break;
            }

            case 0b011001:
            {
                printf("daddiu");
                break;
            }

            case 0b100111:
            {
                printf("lwu");
                break;
            }

            case 0b000010:
            {
                printf("j");
                break;
            }

            case 0b001000:
            {
                printf("addi");
                break;
            }

            case 0b011000:
            {
                printf("daddi");
                break;
            }

            case 0b001101:
            {
                printf("ori");
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

void gen_regimm_lut(const char *prefix)
{
    for(int i = 0; i < 32; i++)
    {
        printf("&%s",prefix);
        switch(i)
        {
            // bgezl
            case 0b00011:
            {
                printf("bgezl");
                break;
            }

            // bgezal
            case 0b10001:
            {
                printf("bgezal");
                break;
            }


            default:
            {
                printf("unknown_regimm");
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

void gen_r_lut(const char *prefix)
{
    for(int i = 0; i < 64; i++)
    {
        printf("&%s",prefix);

        switch(i)
        {
            case 0b000010:
            {
                printf("srl");
                break;
            }

            case 0b000110:
            {
                printf("srlv");
                break;
            }

            case 0b000100:
            {
                printf("sllv");
                break;
            }

            case 0b100110:
            {
                printf("xor");
                break;
            }


            case 0b100011:
            {
                printf("subu");
                break;
            }

            case 0b100001:
            {
                printf("addu");
                break;
            }

            case 0b100000:
            {
                printf("add");
                break;
            }

            case 0b100100:
            {
                printf("and");
                break;
            }

            case 0b000000:
            {
                printf("sll");
                break;
            }

            case 0b011001:
            {
                printf("multu");
                break;
            }

            case 0b010010:
            {
                printf("mflo");
                break;
            }

            case 0b101011:
            {
                printf("sltu");
                break;
            }

            case 0b101010:
            {
                printf("slt");
                break;
            }

            case 0b100101:
            {
                printf("or");
                break;
            }

            case 0b001000:
            {
                printf("jr");
                break;
            }

            case 0b001001:
            {
                printf("jalr");
                break;
            }


            case 0b001111:
            {
                printf("sync");
                break;
            }


            default:
            {
                printf("unknown_r");
                break;
            }
        }

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




    // and r format

    printf("const INSTR_FUNC instr_r_lut[] = {\n");

    // gen the instr table
    gen_r_lut("instr_");

    printf("};\n\n\n");



    printf("const DISASS_FUNC disass_r_lut[] = {\n");

    // gen the disass table
    gen_r_lut("disass_");

    printf("};\n\n\n");


    // regimm
    printf("const INSTR_FUNC instr_regimm_lut[] = {\n");

    // gen the instr table
    gen_regimm_lut("instr_");

    printf("};\n\n\n");



    printf("const DISASS_FUNC disass_regimm_lut[] = {\n");

    // gen the disass table
    gen_regimm_lut("disass_");

    printf("};\n\n\n");


    printf("}\n");
}