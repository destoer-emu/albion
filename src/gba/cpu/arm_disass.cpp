#include <gba/disass.h>
#include <gba/arm.h>
#include <gba/cpu.h>
#include <gba/memory.h>

namespace gameboyadvance
{


void Disass::init_arm_disass_table()
{
    disass_arm_table.resize(4096);

    for(int i = 0; i < 4096; i++)
    {
        switch(i >> 10) // bits 27 and 26 of opcode
        {
            case 0b00:
            {

                int op = (i >> 5) & 0xf;


                //  ARM.7: Multiply and Multiply-Accumulate (MUL,MLA)
                if(((i & 0b1111) == 0b1001) && ((i >> 7) & 0b111111) == 0b000000) 
                {
                    disass_arm_table[i] = &Disass::disass_arm_mul;
                }

                //  multiply and accumulate long
                else if(((i & 0b1111) == 0b1001) && ((i >> 7) & 0b111111) == 0b000001) 
                {
                    disass_arm_table[i] = &Disass::disass_arm_mull;
                }

                // Single Data Swap (SWP)  
                 else if(((i & 0b1111) == 0b1001) && ((i >> 7) & 0b11111) == 0b00010 && ((i >> 4) & 0b11) == 0b00)
                {
                    disass_arm_table[i] = &Disass::disass_arm_swap;
                }



                // ARM.10: Halfword, Doubleword, and Signed Data Transfer
                // (may require more stringent checks than this)
                else if(((i >> 9) & 0b111) == 0b000 && is_set(i,3) && is_set(i,0))
                {
                    disass_arm_table[i] = &Disass::disass_arm_hds_data_transfer;
                }



                //ARM.3: Branch and Exchange
                // bx
                else if(i == 0b000100100001)
                {
                    disass_arm_table[i] = &Disass::disass_arm_branch_and_exchange;
                }


                // msr and mrs
                // ARM.6: PSR Transfer
                // bit 24-23 must be 10 for this instr
                // bit 20 must also be zero
                // check it ocupies the unused space for
                //TST,TEQ,CMP,CMN with a S of zero
                else if(op >= 0x8 && op <= 0xb && !is_set(i,4))
                {
                    disass_arm_table[i] = &Disass::disass_arm_psr;
                }

                //  ARM.5: Data Processing 00 at bit 27
                else
                {
                    disass_arm_table[i] = &Disass::disass_arm_data_processing;
                }
                break;
            }

            case 0b01:
            {
                //ARM.9: Single Data Transfer
                if(true) // assume for now
                {
                    disass_arm_table[i] = &Disass::disass_arm_single_data_transfer;
                }

                else 
                {
                    disass_arm_table[i] = &Disass::disass_arm_unknown;
                }
                break;
            }

            case 0b10:
            {

                // 101 (ARM.4: Branch and Branch with Link)
                if(is_set(i,9)) // if bit 25 set
                {
                    disass_arm_table[i] = &Disass::disass_arm_branch;
                }

                // 100
                // ARM.11: Block Data Transfer (LDM,STM)
                else if(!is_set(i,9))
                {
                    disass_arm_table[i] = &Disass::disass_arm_block_data_transfer;
                }

                else 
                {
                    disass_arm_table[i] = &Disass::disass_arm_unknown;
                }

                break;
            }

            case 0b11:
            {
                disass_arm_table[i] = &Disass::disass_arm_unknown;
                break;
            }
        }        
    }
}



std::string Disass::disass_arm(uint32_t program_counter)
{
    pc = program_counter;
    uint32_t opcode = mem->read_mem<uint32_t>(pc);
    pc += ARM_WORD_SIZE;
    uint32_t op = get_arm_opcode_bits(opcode);

    return std::invoke(disass_arm_table[op],this,opcode);
}



std::string Disass::disass_arm_get_cond_suffix(int opcode)
{
    const int cond_bits = (opcode >> 28) & 0xf;
    return std::string(suf_array[cond_bits]);
}

/* TODO */
std::string Disass::disass_arm_mull(uint32_t opcode)
{
    UNUSED(opcode); return "MULL UNDEFINED";
}

std::string Disass::disass_arm_mul(uint32_t opcode)
{
    int rn = (opcode >> 12) & 0xf;
    int rd = (opcode >> 16) & 0xf;
    int rs = (opcode >> 8) & 0xf;
    int rm = opcode & 0xf;
    bool s = is_set(opcode,20);
    bool a = is_set(opcode,21);

    std::string suffix = disass_arm_get_cond_suffix(opcode);
    if(s)
    {
        suffix += "s";
    }

    if(a) // mla
    {
        return fmt::format("mla{} {},{},{},{}",suffix,
            user_regs_names[rd],user_regs_names[rm],
            user_regs_names[rs],user_regs_names[rn]);
    }

    else // mul
    {
        return fmt::format("mul{} {},{},{}",suffix,
            user_regs_names[rd],user_regs_names[rm],
            user_regs_names[rs]);
    }

}

std::string Disass::disass_arm_swap(uint32_t opcode)
{
    int rm = opcode & 0xf;
    int rd = (opcode >> 12) & 0xf;
    int rn = (opcode >> 16) & 0xf;

    std::string suffix = disass_arm_get_cond_suffix(opcode);

    // byte swp
    if(is_set(opcode,22))
    {
        suffix += "b";
    }

    return fmt::format("swp{} {},{},{}",suffix,user_regs_names[rd],
        user_regs_names[rm],user_regs_names[rn]);    
}

std::string Disass::disass_arm_block_data_transfer(uint32_t opcode)
{
    int addressing_mode = (opcode >> 23) & 0x3;
    bool s = is_set(opcode,22); // psr or force user mode
    bool w = is_set(opcode,21);
    bool l = is_set(opcode,20);
    int rn = (opcode >> 16) & 0xf;
    int rlist = opcode & 0xffff;

    const static char *names[4] = {"da","ia","db","ib"};

    std::string suffix = names[addressing_mode];

    std::string instr = l? "ldm" : "stm";

    std::string reg_str = "{";


    for(int i = 0; i < 16; i++)
    {
        if(is_set(rlist,i))
        {
            reg_str += fmt::format("{},",user_regs_names[i]);
        }
    }

    reg_str[reg_str.size()-1] = '}';

    return fmt::format("{}{} {}{},{}{}",instr,suffix,user_regs_names[rn],
        w? "!" : "", reg_str, s? "^" : "");

}

std::string Disass::disass_arm_hds_data_transfer(uint32_t opcode)
{
    bool p = is_set(opcode,24);
    bool u = is_set(opcode,23);
    bool i = is_set(opcode,22);
    bool l = is_set(opcode,20);
    int rn = (opcode >> 16) & 0xf;
    int rd = (opcode >> 12) & 0xf;
    int op = (opcode >> 5) & 0x3;

    std::string suffix = disass_arm_get_cond_suffix(opcode);




    std::string addr_str;
    
    if(p) // pre indexed
    {
        bool w = is_set(opcode,21);
        if(i)
        {
            uint8_t imm = opcode & 0xf;
            imm |= (opcode >> 4) & 0xf0;
            addr_str = fmt::format("[{},{}#0x{:02x}]{}",user_regs_names[rn],
                u? "" : "-",imm,w? "!" : "");
        }

        else
        {
            int rm = opcode & 0xf;
            addr_str = fmt::format("[{},{}{}]{}",user_regs_names[rn],
                u? "" : "-",user_regs_names[rm],w? "!" : "");
        }
    }

    else // post indexed
    {
        if(i)
        {
            uint8_t imm = opcode & 0xf;
            imm |= (opcode >> 8) & 0xf;
            addr_str = fmt::format("[{}], {}#0x{:02x}",user_regs_names[rn],
                u? "" : "-",imm);
        }

        else
        {
            int rm = opcode & 0xf;
            addr_str = fmt::format("[{}], {}{}",user_regs_names[rn],
                u? "" : "-",user_regs_names[rm]);
        }
    }

    // now do load store and switch on the opcode
    if(l) // load
    {
        switch(op)
        {
            case 0:
            {
                return "hds_load_undefined";
            }

            case 1:
            {
                return fmt::format("ldr{}h {},{}",suffix,user_regs_names[rd],addr_str);
            }

            case 2:
            {
                return fmt::format("ldr{}sb {},{}",suffix,user_regs_names[rd],addr_str);
            }

            case 3:
            {
                return fmt::format("ldr{}sh {},{}",suffix,user_regs_names[rd],addr_str);
            }
        }
    }

    else // store
    {
        switch(op)
        {
            case 0:
            {
                return "hds_store_undefined";
            }

            case 1:
            {
                return fmt::format("str{}h {},{}",suffix,user_regs_names[rd],addr_str);
            }

            case 2:
            {
                return fmt::format("ldr{}d {},{}",suffix,user_regs_names[rd],addr_str);
            }

            case 3:
            {
                return fmt::format("str{}d {},{}",suffix,user_regs_names[rd],addr_str);
            }
        }
    }
    puts("disass_arm_hds_data_transfer fell through!?");
    exit(1);
}

// get a shift type diassembeld from an opcode
std::string Disass::disass_arm_get_shift_string(uint32_t opcode)
{
    int type = (opcode >> 5) & 3;
    int rm = opcode & 0xf;
    // shift type on register
    if(is_set(opcode,4))
    {
        int rs = (opcode >> 8) & 0xf;

        //r1, asr r2
        return fmt::format("{},{} {}",user_regs_names[rm],shift_names[type],user_regs_names[rs]);
    }

    else // shift type on 5 bit immediate
    {
        int imm = (opcode >> 7) & 0x1f;
        // egr1, asr #0x2

        if(imm != 0)
        {
            return fmt::format("{},{} #0x{:x}",user_regs_names[rm],shift_names[type],imm);
        }

        else // depending on the shift it has a behavior
        {         
            auto t = static_cast<shift_type>(type); 
            switch(t)
            {
                case shift_type::lsl: // no shift done
                {
                    return std::string(user_regs_names[rm]);
                }

                case shift_type::lsr: // 32 shift
                {
                    return fmt::format("{}, lsr #0x20",user_regs_names[rm]);
                }

                case shift_type::asr: // 32 shift
                {
                    return fmt::format("{}, asr #0x20",user_regs_names[rm]);
                }

                case shift_type::ror: // rrx
                {
                    return fmt::format("{}, rrx #0x1",user_regs_names[rm],32);
                }
            }
        }            
    }
    return ""; 
}


std::string Disass::disass_arm_branch_and_exchange(uint32_t opcode)
{
    std::string suffix = disass_arm_get_cond_suffix(opcode);

    int rn = opcode & 0xf;

    return fmt::format("bx{} {}",suffix,user_regs_names[rn]);
}

// psr transfer 
std::string Disass::disass_arm_psr(uint32_t opcode)
{
    // determine wether we are doing msr or mrs
    // and what kind we are doing
    std::string output;

    bool is_msr = is_set(opcode,21); // 21 set msr else mrs

    std::string suffix = disass_arm_get_cond_suffix(opcode);

    bool spsr = is_set(opcode,22);

    std::string sr = spsr? "spsr" : "cpsr";

    if(is_msr) //msr (unsure this handles the different write operations eg flag only)
    {
        int rm = opcode & 0xf;
        // msr to psr
        if(is_set(opcode,15)) 
        {
            output = fmt::format("msr{} {}, {}",suffix,sr,user_regs_names[rm]); 
        }

        // msr to psr (flag bits only)
        else
        {
            if(is_set(opcode,25))
            {
                output = fmt::format("msr{} {}_flg, {}",suffix,sr,get_arm_operand2_imm(opcode));
            }

            else
            {
                output = fmt::format("msr{} {}_flg, {}",suffix,sr,user_regs_names[rm]);
            } 
        }
    }

    else // mrs
    {
        int rd = (opcode >> 12) & 0xf;
        output = fmt::format("mrs{} {}, {}",suffix,user_regs_names[rd],sr); 
    }
    return output;
}

std::string Disass::disass_arm_data_processing(uint32_t opcode)
{
    // get the register and immediate
    std::string suffix = disass_arm_get_cond_suffix(opcode);




    int rd = (opcode >> 12) & 0xf;

    int rn = (opcode >> 16) & 0xf;

    // if i bit its an immediate else its a shifted reg or value
    std::string operand2 = is_set(opcode,25)? fmt::format("#0x{:08x}",get_arm_operand2_imm(opcode)) : disass_arm_get_shift_string(opcode); 

    // what instr is it
    int op = (opcode >> 21) & 0xf;

    if(is_set(opcode,20) && !(op >= 8 && op <= 0xb)) // if s bit is set and writes back to a dest
    {
        suffix += 's';
    }

    switch((opcode >> 21) & 0xf)
    {

        case 0x0: // and
        {
            return fmt::format("and{} {},{},{}",suffix,user_regs_names[rd],user_regs_names[rn],operand2);
            break;           
        }

        case 0x1: // eor
        {
            return fmt::format("eor{} {},{},{}",suffix,user_regs_names[rd],user_regs_names[rn],operand2);
            break;               
        }

        case 0x2: // sub
        {
            return fmt::format("sub{} {},{},{}",suffix,user_regs_names[rd],user_regs_names[rn],operand2);
            break;            
        }

        case 0x3: // rsb
        {
            return fmt::format("rsb{} {},{},{}",suffix,user_regs_names[rd],user_regs_names[rn],operand2);
            break;            
        }        

        case 0x4: // add
        {
            return fmt::format("add{} {},{},{}",suffix,user_regs_names[rd],user_regs_names[rn],operand2);
            break;
        }

        case 0x5: // adc
        {
            return fmt::format("adc{} {},{},{}",suffix,user_regs_names[rd],user_regs_names[rn],operand2);
            break;           
        }

        case 0x6: //sbc
        {
            return fmt::format("sbc{} {},{},{}",suffix,user_regs_names[rd],user_regs_names[rn],operand2);
            break;
        }

        case 0x7: // rsc
        {
            return fmt::format("rsc{} {},{},{}",suffix,user_regs_names[rd],user_regs_names[rn],operand2);
            break;
        }

        case 0x8: // tst
        {
            return fmt::format("tst{} {},{}",suffix,user_regs_names[rn],operand2);
            break;
        }

        case 0x9: // teq
        {
            return fmt::format("teq{} {},{}",suffix,user_regs_names[rn],operand2);
            break;
        }

        case 0xa: // cmp
        {
            return fmt::format("cmp{} {},{}",suffix,user_regs_names[rn],operand2);
            break;
        }

        case 0xb: // cmn
        {
            return fmt::format("cmn{} {},{}",suffix,user_regs_names[rn],operand2);
            break;
        }


        case 0xc: // orr
        {
            return fmt::format("orr{} {},{},{}",suffix,user_regs_names[rd],user_regs_names[rn],operand2);
            break;
        }

        case 0xd: // mov
        {
            return fmt::format("mov{} {},{}",suffix,user_regs_names[rd],operand2);
            break;
        }

        case 0xe: // bic
        {
            return fmt::format("bic{} {},{},{}",suffix,user_regs_names[rd],user_regs_names[rn],operand2);
            break;                       
        }

        case 0xf: // mvn
        {
            return fmt::format("mvn{} {},{}",suffix,user_regs_names[rd],operand2);
            break;            
        }
    }
    return "";
}

// negative calc on a branch messes up horribly
std::string Disass::disass_arm_branch(uint32_t opcode)
{
    std::string output;


    // 24 bit offset is shifted left 2
    // and extended to a 32 bit int
    int32_t offset = (opcode & 0xffffff) << 2;
    offset = sign_extend(offset,26);

    uint32_t addr = (pc+ARM_WORD_SIZE) + offset;


    std::string suffix = disass_arm_get_cond_suffix(opcode);


    // if the link bit is set this acts as a call instr
    if(is_set(opcode,24))
    {
        output = fmt::format("bl{} #0x{:08x}",suffix,addr);
    }


    else 
    {
        output = fmt::format("b{} #0x{:08x}",suffix,addr);
    }

    return output;
}

// str, ldr

// when u get back to this handle post indexed and the register offset
std::string Disass::disass_arm_single_data_transfer(uint32_t opcode)
{
    
    std::string output;

    // get the instr (load / store bit)
    std::string instr = is_set(opcode,20)? "ldr" : "str";
    std::string suffix = disass_arm_get_cond_suffix(opcode);


    suffix += is_set(opcode,22) ? "b" : ""; // is byte str

    int rd = (opcode >> 12) & 0xf;
    int base = (opcode >> 16) & 0xf;


    std::string imm;

    // register offset
    if(is_set(opcode,25))
    {
        imm = disass_arm_get_shift_string(opcode);
    }

    else // immeditate
    {
        // 12 bit immediate
        int v = opcode & 0xfff;
        imm = fmt::format("#0x{:x}",v);
    }

    if(is_set(opcode,24)) // pre indexed
    {
        // write back enabled?
        std::string w = is_set(opcode,21) ? "!" : ""; 
        output = fmt::format("{}{} {},[{},{}]{}",instr,suffix,
            user_regs_names[rd],user_regs_names[base],imm,w);
    }

    else // post indexed
    {
        // if post indexed add a t if writeback set
        // this forces user mode in a system context
        suffix += is_set(opcode,21) ? "t" : "";
        output = fmt::format("{}{} {},[{}],{}",instr,suffix,user_regs_names[rd],
            user_regs_names[base],imm);
    }

    return output;
}


std::string Disass::disass_arm_unknown(uint32_t opcode)
{
    /*
    uint32_t op = ((opcode >> 4) & 0xf) | ((opcode >> 16) & 0xff0);
    printf("[disass-arm]%08x:unknown opcode %08x:%08x\n",pc,opcode,op);
    cpu->print_regs();
    exit(1);
    */
    UNUSED(opcode);
    return "unknown opcode";      
}

}