#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

template<typename T>
inline bool is_set(T reg, int bit) noexcept
{
	return ((reg >> bit) & 1);
}

void print_arm_fptr(int i)
{
    switch(i >> 10) // bits 27 and 26 of i
    {
    
        case 0b00:
        {

            // 001
            if(is_set(i,9)) 
            {
                int op = (i >> 5) & 0xf;

                // msr and mrs
                // ARM.6: PSR Transfer
                // bit 24-23 must be 10 for this instr 
                // bit 20 must also be zero
                
                // check it ocupies the unused space for
                //TST,TEQ,CMP,CMN with a S of zero                    
                if(op >= 0x8 && op <= 0xb && !is_set(i,4))
                {
                    const bool MSR = is_set(i,21-16); // 21 set msr else mrs
                    const bool SPSR = is_set(i,22-16); // to cpsr or spsr?
                    const bool I = is_set(i,25-16);
                    
                    printf("&Cpu::arm_psr<%d,%d,%d>,\n",MSR,SPSR,I);
                }

                //  ARM.5: Data Processing 00 at bit 27
                // arm data processing immediate
                else
                {
                    const bool S = is_set(i,20-16);
                    const bool I = is_set(i,25-16);
                    const int OP = (i >> (21-16)) & 0xf;
                    printf("&Cpu::arm_data_processing<%d,%d,%d>,\n",S,I,OP);
                }
            }

            // 000
            else 
            {
                //ARM.3: Branch and Exchange
                // bx
                if(i == 0b000100100001) 
                {
                    printf("&Cpu::arm_branch_and_exchange,\n");
                }

                // this section of the decoding needs improving....
                else if((i & 0b1001) == 0b1001)
                {
                    // ARM.7: Multiply and Multiply-Accumulate (MUL,MLA)
                    if(((i >> 6) & 0b111) == 0b000 && (i & 0xf) == 0b1001)
                    {
                        const bool S = is_set(i,20-16);
                        const bool A = is_set(i,21-16);
                        printf("&Cpu::arm_mul<%d,%d>,\n",S,A);
                    }

                    // ARM.7: Multiply and Multiply-Accumulate (MUL,MLA) (long)
                    else if(((i >> 7) & 0b11) == 0b01 && (i & 0xf) == 0b1001)
                    {
                        //lut[i] = &Cpu::arm_mull;
                        const bool S = is_set(i,20-16);
                        const bool A = is_set(i,21-16);
                        const bool U = !is_set(i,22-16);


                        printf("&Cpu::arm_mull<%d,%d,%d>,\n",S,A,U);
                    }
                    

                    // Single Data Swap (SWP)  
                    else if(is_set(i,8) && (i & 0xf) == 0b1001) // bit 24 set
                    {
                        const bool B = is_set(i,22-16);
                        printf("&Cpu::arm_swap<%d>,\n",B);
                    }

                    // ARM.10: Halfword, Doubleword, and Signed Data Transfer
                    //else if()
                    else 
                    {
                        const bool P = is_set(i,24-16);
                        const bool U = is_set(i,23-16);
                        const bool I = is_set(i,22-16);
                        const bool L = is_set(i,20-16);
                        const bool W = is_set(i,21-16);
                        
                        printf("&Cpu::arm_hds_data_transfer<%d,%d,%d,%d,%d>,\n",P,U,I,L,W);
                    }
                }

                // psr or data processing
                else
                {
                    int op = (i >> 5) & 0xf;
                    // check it ocupies the unused space for
                    //TST,TEQ,CMP,CMN with a S of zero 
                    // ARM.6: PSR Transfer                   
                    if(op >= 0x8 && op <= 0xb && !is_set(i,4))
                    {
                        const bool MSR = is_set(i,21-16); // 21 set msr else mrs
                        const bool SPSR = is_set(i,22-16); // to cpsr or spsr?
                        const bool I = is_set(i,25-16);
                        
                        printf("&Cpu::arm_psr<%d,%d,%d>,\n",MSR,SPSR,I);
                    }

                    //  ARM.5: Data Processing 00 at bit 27
                    // arm data processing register
                    else
                    {
                        const bool S = is_set(i,20-16);
                        const bool I = is_set(i,25-16);
                        const int OP = (i >> (21-16)) & 0xf;
                        printf("&Cpu::arm_data_processing<%d,%d,%d>,\n",S,I,OP);                        
                    } 
                }                   
            }
            break;
        }
    

    
        case 0b01:
        {
            //ARM.9: Single Data Transfer
            if(true) // assume for now
            {
                const bool L = is_set(i,20-16);
                const bool W = is_set(i,21-16); // write back
                const bool P = is_set(i,24-16); // pre index
                const bool I = is_set(i,25-16);
                
                printf("&Cpu::arm_single_data_transfer<%d,%d,%d,%d>,\n",L,W,P,I);   
            }

            else 
            {
                printf("&Cpu::arm_unknown,\n");
            }
            break;
        }
    
        case 0b10:
        {

            // 101 (ARM.4: Branch and Branch with Link)
            if(is_set(i,9))
            {
                const bool L = is_set(i,24 - 16);
                printf("&Cpu::arm_branch<%d>,\n",L);
            }

            
            // 100
            // ARM.11: Block Data Transfer (LDM,STM)
            else if(!is_set(i,9))
            {
                
                bool P = is_set(i,24-16);
                const bool U = is_set(i,23-16);
                const bool S = is_set(i,22-16); // psr or force user mode
                bool W = is_set(i,21-16);
                const bool L = is_set(i,20-16);

                // allways adding on address so if  we are in "down mode"
                // we need to precalc the buttom1
                if(!U) 
                {
                    // invert the pre/post
                    // as predoing the addr has messed with the meaning
                    P = !P;  
                }

                printf("&Cpu::arm_block_data_transfer<%d,%d,%d,%d,%d>,\n",S,P,U,W,L);
            }
            
            break;
        }
        

        
        case 0b11:
        {

            // 1111 SWI
            if(((i >> 8) & 0b1111) == 0b1111)
            {
                printf("&Cpu::arm_swi,\n");
            }

            // rest are coprocesor instrucitons and are undefined on the gba
            else
            {
                printf("&Cpu::arm_unknown,\n");
            }
            break;
        }

        default:
        {
            printf("&Cpu::arm_unknown,\n");
        }
        
    }
}

int main()
{
    printf("// horrible fleroviux template hacks go brrrrr\n");
    printf("#pragma once\n#include <gba/cpu.h>\n");
    printf("namespace gameboyadvance\n{\n");
    printf("const ARM_OPCODE_LUT arm_opcode_table = \n{\n");
    for(int i = 0; i < 4096; i++)
    {
        printf("\t");
        print_arm_fptr(i);
    }
    printf("};\n}\n");
}
