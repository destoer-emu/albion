#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

template<typename T>
inline bool is_set(T reg, int bit) noexcept
{
	return ((reg >> bit) & 1);
}

void print_opcode_table()
{
   printf("using OPCODE_HANDLER = void (Cpu::*)(void);\n");
    printf("const OPCODE_HANDLER opcode_table[256] = \n{\n");
    for(int i = 0; i <= 0xff; i++)
    {
        if(i == 0b00000000)
        {
            printf("&Cpu::nop");
        }

        // LD (u16), SP
        else if(i == 0b00001000)
        {
            printf("&Cpu::ld_u16_sp");
        }

        // jp u16
        else if((i & 0b11000111) == 0b11000011)
        {
            switch((i >> 3) & 0b111)
            {
                case 1: printf("&Cpu::cb_opcode"); break;

                case 0: printf("&Cpu::jp"); break;

                case 6: printf("&Cpu::di"); break;

                case 7: printf("&Cpu::ei"); break;

                default: printf("&Cpu::undefined_opcode"); break;
            }
        }

        // ld r16, u16
        else if((i & 0b11001111) == 0b00000001)
        {
            const int reg = (i >> 4) & 0b11;
            printf("&Cpu::ld_r16_u16<%d>",reg);
        }

        // ld (u16), a
        else if(i == 0b11101010)
        {
            printf("&Cpu::ld_u16_a");
        }

        // ld a, u16
        else if(i == 0b11111010)
        {
            printf("&Cpu::ld_a_u16");
        } 

        // ld (ff00+u8),a
        else if(i == 0b11100000)
        {
            printf("&Cpu::ld_ffu8_a");
        }

        // call u16
        else if(i == 0b11001101)
        {
            printf("&Cpu::call");
        }

        // Halt (must come before ld_r8_r8)
        else if(i == 0b01110110)
        {
            printf("&Cpu::halt");
        }


        // ld r8, r8
        else if((i & 0b11000000) == 0b01000000)
        {
            const int DST = (i >> 3) & 0b111;
            const int SRC = (i >> 0) & 0b111;
            printf("&Cpu::ld_r8_r8<%d,%d>",DST,SRC);
        } 

        // jr rel_addr
        else if(i == 0b00011000)
        {
            printf("&Cpu::jr");
        }

        else if((i & 0b11001111) == 0b11001001)
        {
            switch((i >> 4) & 0b11)
            {
                case 0: printf("&Cpu::ret"); break;
                case 1: printf("&Cpu::reti"); break;
                case 2: printf("&Cpu::jp_hl"); break;
                case 3: printf("&Cpu::ld_sp_hl"); break;

                default: printf("&Cpu::undefined_opcode");
            }
        }


        // ld r8, u8
        else if((i & 0b11000111) == 0b00000110)
        {
            const int REG = (i >> 3) & 0b111;
            printf("&Cpu::ld_r8_u8<%d>",REG);
        }

        // push r16
        else if((i & 0b11001111) == 0b11000101)
        {
            const int REG = (i >> 4) & 0b11;
            printf("&Cpu::push<%d>",REG);
        }

        // pop r16
        else if((i & 0b11001111) == 0b11000001)
        {
            const int REG = (i >> 4) & 0b11;
            printf("&Cpu::pop<%d>",REG);
        }

        // inc r16
        else if((i & 0b11001111) == 0b00000011)
        {
            const int REG = (i >> 4);
            printf("&Cpu::inc_r16<%d>",REG);
        }

        // ld a, (r16)
        else if((i & 0b11001111) == 0b00001010)
        {
            const int REG = (i >> 4);
            printf("&Cpu::ld_a_r16<%d>",REG);
        }


        // alu a, r8
        else if((i & 0b11000000) == 0b10000000)
        {
            const int REG = (i >> 0) & 0b111;
            switch((i >> 3) & 0b111)
            {
                case 0: printf("&Cpu::add_r8<%d>",REG); break;
                case 1: printf("&Cpu::adc_r8<%d>",REG); break;
                case 2: printf("&Cpu::sub_r8<%d>",REG); break;
                case 3: printf("&Cpu::sbc_r8<%d>",REG); break;
                case 4: printf("&Cpu::and_r8<%d>",REG); break;
                case 5: printf("&Cpu::xor_r8<%d>",REG); break;
                case 6: printf("&Cpu::or_r8<%d>",REG); break;
                case 7: printf("&Cpu::cp_r8<%d>",REG); break;

                default: printf("&Cpu::undefined_opcode"); break;
            }
        }

        // alu a, u8
        else if((i & 0b11000111) == 0b11000110)
        {
            switch((i >> 3) & 0b111)
            {
                case 0: printf("&Cpu::add_u8"); break;
                case 1: printf("&Cpu::adc_u8"); break;
                case 2: printf("&Cpu::sub_u8"); break;
                case 3: printf("&Cpu::sbc_u8"); break;
                case 4: printf("&Cpu::and_u8"); break;
                case 5: printf("&Cpu::xor_u8"); break;
                case 6: printf("&Cpu::or_u8"); break;
                case 7: printf("&Cpu::cp_u8"); break;

                default: printf("&Cpu::undefined_opcode"); break;
            }
        }


        // jr<cond> rel
        else if((i & 0b11100111) == 0b00100000)
        {
            const int COND = (i >> 3) & 0b11;
            printf("&Cpu::jr_cond<%d>",COND);
        }

        // ld a, (ff00+u8)
        else if(i == 0b11110000)
        {
            printf("&Cpu::ld_a_ffu8");
        }

        // call<cond>, u16
        else if((i & 0b11100111) == 0b011000100)
        {
            const int COND = (i >> 3) & 0b11;
            printf("&Cpu::call_cond<%d>",COND);            
        }

        // dec r8
        else if((i & 0b11000111) == 0b00000101)
        {
            const int REG = (i >> 3) & 0b111;
            printf("&Cpu::dec_r8<%d>",REG);
        }

        // inc r8
        else if((i & 0b11000111) == 0b00000100)
        {
            const int REG = (i >> 3) & 0b111;
            printf("&Cpu::inc_r8<%d>",REG);
        }

        // ld_r16_a
        else if((i & 0b11001111) == 0b00000010)
        {
            const int REG = (i >> 4);
            printf("&Cpu::ld_r16_a<%d>",REG);            
        }

        // opcode table group one
        else if((i & 0b11000111) == 0b00000111)
        {
            switch((i >> 3) & 0b111)
            {
                case 0: printf("&Cpu::rlca"); break;
                case 1: printf("&Cpu::rrca"); break;
                case 2: printf("&Cpu::rla"); break;
                case 3: printf("&Cpu::rra"); break;
                case 4: printf("&Cpu::daa"); break;
                case 5: printf("&Cpu::cpl"); break;
                case 6: printf("&Cpu::scf"); break;
                case 7: printf("&Cpu::ccf"); break;

                default: printf("&Cpu::undefined_opcode"); break;
            }
        }

        // ret<cond> u16
        else if((i & 0b11100111) == 0b11000000)
        {
            const int COND = (i >> 3) & 0b11;
            printf("&Cpu::ret_cond<%d>",COND);
        }

        // add hl, r16
        else if((i & 0b11001111) == 0b00001001)
        {
            const int REG = (i >> 4) & 0b11;
            printf("&Cpu::add_hl_r16<%d>",REG);
        }

        else if((i & 0b11100111) == 0b11000010)
        {
            const int COND = (i >> 3) & 0b11;
            printf("&Cpu::jp_cond<%d>",COND);
        }

        else if(i == 0b11111000)
        {
            printf("&Cpu::ld_hl_sp_i8");
        }

        else if(i == 0b00010000)
        {
            printf("&Cpu::stop");
        }

        else if((i & 0b11001111) == 0b00001011)
        {
            const int REG = (i >> 4) & 0b11;
            printf("&Cpu::dec_r16<%d>",REG);
        }

        else if(i == 0b11101000)
        {
            printf("&Cpu::add_sp_i8");
        }

        else if((i & 0b11000111) == 0b11000111)
        {
            const int ADDR = (i & 0b00111000);
            const uint8_t OP = i;
            printf("&Cpu::rst<0x%02x,0x%02x>",ADDR,OP);
        }

        // ld a, (ff00+c)
        else if(i == 0b11110010)
        {
            printf("&Cpu::ld_a_ff00_c");
        }

        // ld (ff00+c)
        else if(i == 0b11100010)
        {
            printf("&Cpu::ld_ff00_c_a");
        }

        else
        {
            printf("&Cpu::undefined_opcode");
        }

        printf(", // %02x\n",i);
    }
    printf("};\n");    
}

void print_cb_table()
{
    printf("using OPCODE_HANDLER = void (Cpu::*)(void);\n");
    printf("const OPCODE_HANDLER cb_table[256] = \n{\n");
    for(int i = 0; i <= 0xff; i++)
    {
        const int REG = (i >> 0) & 0b111;
        switch((i >> 6) & 0b11)
        {
            case 0b00:
            {
                switch((i >> 3) & 0b111)
                {
                    case 0: printf("&Cpu::rlc_r8<%d>",REG); break;
                    case 1: printf("&Cpu::rrc_r8<%d>",REG); break;
                    case 2: printf("&Cpu::rl_r8<%d>",REG); break;
                    case 3: printf("&Cpu::rr_r8<%d>",REG); break;
                    case 4: printf("&Cpu::sla_r8<%d>",REG); break;
                    case 5: printf("&Cpu::sra_r8<%d>",REG); break;
                    case 6: printf("&Cpu::instr_swap<%d>",REG); break;
                    case 7: printf("&Cpu::srl<%d>",REG); break;
                    default: printf("&Cpu::undefined_opcode_cb"); break;
                }
                break;
            }

            case 0b01:
            {
                const int REG = (i >> 0) & 0b111;
                const int BIT = (i >> 3) & 0b111;
                printf("&Cpu::bit_r8<%d,%d>",REG,BIT);
                break;
            }

            case 0b10:
            {
                const int REG = (i >> 0) & 0b111;
                const int BIT = (i >> 3) & 0b111;
                printf("&Cpu::res_r8<%d,%d>",REG,BIT);
                break;
            }


            case 0b11:
            {
                const int REG = (i >> 0) & 0b111;
                const int BIT = (i >> 3) & 0b111;
                printf("&Cpu::set_r8<%d,%d>",REG,BIT);
                break;
            }


            default: printf("&Cpu::undefined_opcode_cb"); break;
        }

        
        
        printf(", // %02x\n",i);        
    }
    printf("};\n");
}

int main()
{
    printf("#pragma once\n#include <gb/cpu.h>\n");
    printf("namespace gameboy\n{\n");

    print_opcode_table();
    print_cb_table();

    printf("\n}\n");
}