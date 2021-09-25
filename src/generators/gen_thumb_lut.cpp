#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

template<typename T>
inline bool is_set(T reg, int bit) noexcept
{
	return ((reg >> bit) & 1);
}

void print_thumb_fptr(int i)
{   
    // THUMB.1: move shifted register
    // top 3 bits unset
    if(((i >> 7) & 0b111) == 0b000 && ((i >> 5) & 0b11) != 0b11)
    {
        const auto TYPE = ((i >> (11-6)) & 0x3);
        printf("&Cpu::thumb_mov_reg_shift<%d>,\n",TYPE);
    }

    // THUMB.2: add/subtract
    else if(((i >> 5) & 0b11111) == 0b00011)
    {
        const auto OP = (i >> (9-6)) & 0x3;
        printf("&Cpu::thumb_add_sub<%d>,\n",OP);
    }



    // THUMB.3: move/compare/add/subtract immediate
    else if(((i >> 7) & 0b111) == 0b001)
    {
        const auto OP = (i >> (11-6)) & 0x3;
        const auto RD = (i >> (8-6)) & 0x7;
        printf("&Cpu::thumb_mcas_imm<%d,%d>,\n",OP,RD);
    }


    // THUMB.4: ALU operations
    else if(((i >> 4) & 0b111111) == 0b010000)
    {
        const auto OP = i & 0xf;
        printf("&Cpu::thumb_alu<%d>,\n",OP);
    }

    // THUMB.5: Hi register operations/branch exchange
    else if(((i >> 4) & 0b111111) == 0b010001)
    {
        const auto OP = (i >> (8-6)) & 0x3;
        printf("&Cpu::thumb_hi_reg_ops<%d>,\n",OP);
    }

    // THUMB.6: load PC-relative
    else if(((i >> 5) & 0b11111) ==  0b01001)
    {
        const auto RD = (i >> (8-6)) & 0x7;
        printf("&Cpu::thumb_ldr_pc<%d>,\n",RD);
    }


    // THUMB.7: load/store with register offset
    else if(((i >> 6) & 0b1111) == 0b0101 && !is_set(i,3))
    {
        const auto OP = (i >> (10-6)) & 0x3;
        printf("&Cpu::thumb_load_store_reg<%d>,\n",OP);
    }

    // THUMB.8: load/store sign-extended byte/halfword
    else if(((i >> 6) & 0b1111) == 0b0101 && is_set(i,3))
    {
        const auto OP = (i >> (10-6)) & 0x3;
        printf("&Cpu::thumb_load_store_sbh<%d>,\n",OP);
    }

    // THUMB.9: load/store with immediate offset
    else if(((i>>7) & 0b111) == 0b011)
    {
        const auto OP = (i >> (11-6)) & 3;
        printf("&Cpu::thumb_ldst_imm<%d>,\n",OP);
    }



    //THUMB.10: load/store halfword
    else if(((i >> 6) & 0b1111) == 0b1000)
    {
        const bool L = is_set(i,11-6);
        printf("&Cpu::thumb_load_store_half<%d>,\n",L);
    }

    // THUMB.11: load/store SP-relative
    else if(((i >> 6) & 0b1111) == 0b1001)
    {
        const auto RD = (i >> (8 - 6)) & 0x7;
        const bool L = is_set(i,11-6);

        printf("&Cpu::thumb_load_store_sp<%d,%d>,\n",RD,L);
    }

    // THUMB.12: get relative address
    else if(((i >> 6) & 0b1111) == 0b1010)
    {
        const auto RD = (i >> (8-6)) & 0x7;
        const bool IS_PC = !is_set(i,11-6);

        printf("&Cpu::thumb_get_rel_addr<%d,%d>,\n",RD,IS_PC);
    }
    


    // THUMB.13: add offset to stack pointer
    else if((i >> 2) == 0b10110000)
    {
        printf("&Cpu::thumb_sp_add,\n");
    }



    //THUMB.14: push/pop registers
    else if(((i >> 6) & 0b1111) == 0b1011 
        && ((i >> 3) & 0b11) == 0b10)
    {
        const bool POP = is_set(i,11-6);
        const bool IS_LR = is_set(i,8-6);
        printf("&Cpu::thumb_push_pop<%d,%d>,\n",POP,IS_LR);
    }

    //  THUMB.15: multiple load/store
    else if(((i >> 6) & 0b1111) == 0b1100)
    {
        const auto RB = (i >> (8-6)) & 0x7;
        const bool L = is_set(i,11-6);
        printf("&Cpu::thumb_multiple_load_store<%d,%d>,\n",RB,L);
    }

    // THUMB.16: conditional branch
    else if(((i >> 6)  & 0b1111) == 0b1101 && ((i >> 2) & 0xf) != 0xf)
    {
        const auto COND = (i >> (8-6)) & 0xf;
        printf("&Cpu::thumb_cond_branch<%d>,\n",COND);
    }

    // THUMB.17: software interrupt and breakpoint
    else if((i >> 2) == 0b11011111)
    {
        printf("&Cpu::thumb_swi,\n");
    }


    // THUMB.18: unconditional branch
    else if(((i >> 5) & 0b11111) == 0b11100)
    {
        printf("&Cpu::thumb_branch,\n");
    }

    // THUMB.19: long branch with link
    else if(((i >> 6) & 0b1111) == 0b1111)
    {
        const bool FIRST = !is_set(i,11-6);
        printf("&Cpu::thumb_long_bl<%d>,\n",FIRST);
    }

    else 
    {
        printf("&Cpu::thumb_unknown,\n");
    }
}

int main()
{
    printf("// horrible fleroviux template hacks go brrrrr\n");
    printf("#pragma once\n#include <gba/cpu.h>\n");
    printf("namespace gameboyadvance\n{\n");
    printf("const THUMB_OPCODE_LUT thumb_opcode_table = \n{\n");
    for(int i = 0; i < 1024; i++)
    {
        printf("\t");
        print_thumb_fptr(i);
    }
    printf("};\n}\n");
}