#include <gb/gb.h>


// todo text debugger rewrite
// determine arg type int or string (done)
// follwing functions (base) (done)
// disass (done)
// step (done)
// break (done)
// watch 
// trace (done)
// registers (done)
// memory (done)
// clear breakpoints
// disable / enable break points
// disable / enable watch points


namespace gameboy
{

GBDebug::GBDebug(GB &g) : gb(g)
{

}




template<typename F>
bool verify_immediate_internal(const std::string &line, uint32_t &i, F lambda)
{
    const auto len = line.size();

    for(; i < len; i++)
    {
        // valid part of the value
        if(lambda(line[i]))
        {
            continue;
        }

        // values cannot have these at the end!
        else if(isalpha(line[i]))
        {
            return false;
        }

        // we have  < ; + , etc stop parsing
        else 
        {
            return true;
        }
    }

    return true;
}


bool verify_immediate(const std::string &line, std::string &literal)
{
    const auto len = line.size();

    // an empty immediate aint much use to us
    if(!len)
    {
        return false;
    }

    uint32_t i = 0;

    const auto c = line[0];

    // allow - or +
    if(c == '-' || c == '+')
    {
        i = 1;
        // no digit after the sign is of no use
        if(len == 1)
        {
            return false;
        }
    }

    bool valid = false;


    // have prefix + one more digit at minimum
    const auto prefix = i+2 < len?  line.substr(i,2) : "";

    // verify we have a valid hex number
    if(prefix == "0x")
    {
        // skip past the prefix
        i += 2;
        valid = verify_immediate_internal(line,i,[](const char c) 
        {
            return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
        });
    }

    // verify its ones or zeros
    else if(prefix == "0b")
    {
        // skip past the prefix
        i += 2;                
        valid = verify_immediate_internal(line,i,[](const char c) 
        {
            return c == '0' || c == '1';
        });
    }

    // verify we have all digits
    else
    {
        valid = verify_immediate_internal(line,i,[](const char c) 
        {
            return c >= '0' && c <= '9';
        });
    }
    

    if(valid)
    {
        literal = line.substr(0,i);
    }

    return valid;    
}


uint32_t convert_imm(const std::string &imm)
{
    if(imm.size() >= 3 && imm.substr(0,2) == "0b")
    {
        return static_cast<uint32_t>(std::stoi(imm.substr(2),0,2));
    }

    // stoi wont auto detect base for binary strings?
    return static_cast<uint32_t>(std::stoi(imm,0,0));
}

bool decode_imm(const std::string &line, uint32_t &i,std::string &literal)
{
    const auto success = verify_immediate(line.substr(i),literal);

    // set one back for whatever the terminating character was
    i--;

    i += literal.size();  

    return success;
}

bool GBDebug::process_args(const std::string &line,std::vector<CommandArg> &args, std::string &command)
{
    size_t argc = 0;
    command = "";
    args.clear();
    for(uint32_t i = 0; i < line.size(); i++)
    {
        const auto c =  line[i];
        switch(c)
        {
            case ' ': break;
            case '\n': break;
            case '\t': break;
            case '\r': break;
            case '\0': break;

         
            default:
            {   
                arg_type type;
                std::string literal = "";

                // integer
                if(isdigit(c))
                {
                    type = arg_type::integer;
                    if(!decode_imm(line,i,literal))
                    {
                        return false;
                    }
                }

                // string
                else
                {
                    type = arg_type::string;
                    for(; i < line.size(); i++)
                    {
                        const auto c = line[i];
                        if(c == ' ')
                        {
                            break;
                        }

                        literal += c;
                    }
                }


                // first arg is the command
                if(argc == 0)
                {
                    command = literal;
                }

                // push as arg
                else
                {
                    args.push_back(CommandArg(literal,type));
                }


                argc++;
                break;
            }
        }
    }
    return true;
}


void GBDebug::execute_command(const std::string &command, const std::vector<CommandArg> &args)
{
    if(!func_table.count(command))
    {
        print_console("unknown command: '{}'\n",command);
        return;
    }

    const auto func = func_table[command];

    std::invoke(func,this,args);
}

// for use under SDL i dont know how we want to do the one for imgui yet...
void GBDebug::debug_input()
{
    printf("%04x\n",gb.cpu.read_pc());
    std::string line = "";


    std::vector<CommandArg> args;
    std::string command = "";
    quit = false;
    while(!quit)
    {
        printf("$ ");
        std::getline(std::cin,line);

        // lex the line and pull the command name along with the args.
        if(!process_args(line,args,command))
        {
            // TODO: provide better error reporting
            puts("one or more args is invalid");
        }
        
        execute_command(command,args);
    }
}
void GBDebug::breakpoint(const std::vector<CommandArg> &args)
{
    if(!args.size() || args.size() > 3)
    {
        print_console("usage: break <addr> . <value> . <type> . \n",args.size());
        return;
    }

    gb.change_breakpoint_enable(true); // TODO remove this

    if(args[0].type != arg_type::integer)
    {
        print_console("expected int got string: {}\n",args[0].literal);
        return;
    }

    const auto addr = convert_imm(args[0].literal);

    bool r = false;
    bool w = false;
    bool x = true;
    bool value_enabled = false;

    auto value = 0xdeadbeef;

    // for 2nd arg allow tpye or value
    if(args.size() >= 2)
    {
        if(args[1].type == arg_type::integer)
        {
            value = convert_imm(args[1].literal);
            value_enabled = true;
        }

        // type set
        else
        {
            for(const auto c: args[1].literal)
            {
                switch(c)
                {
                    case 'r': r = true; break;
                    case 'w': w = true; break;
                    case 'x': x = true; break;

                    default:
                    {
                        print_console("expected break type (rwx) got: {}\n",args[1].literal);
                        return;
                    }
                }
            }

            // optinal value after the type
            if(args.size() == 3)
            {
                // last is value and previous was type set
                if(args[2].type == arg_type::integer)
                {
                    value = convert_imm(args[1].literal);
                    value_enabled = true;
                }

                else
                {
                    print_console("expected int got string: {}\n",args[0].literal);
                    return;
                }
            }
        }
    }

    set_breakpoint(addr,r,w,x,value_enabled,value);
    print_console("breakpoint set at: {:x}\n",addr);
}


void GBDebug::run(const std::vector<CommandArg> &args)
{
    UNUSED(args);
    wake_up();
    quit = true;
    print_console("resuming execution\n");
}

void GBDebug::regs(const std::vector<CommandArg> &args)
{
    UNUSED(args);
    auto &cpu = gb.cpu;
    print_console("CPU REGS\n\nPC:{:04x}\nAF:{:04x}\nBC:{:04x}\nDE:{:04x}\nHL:{:04x}\nSP:{:04x}\n"
            ,cpu.read_pc(),cpu.read_af(),cpu.read_bc(),cpu.read_de(),cpu.read_hl(),cpu.read_sp());

    print_console("\nFLAGS\nC: {}\nH: {}\nN: {}\nZ: {}\n"
        ,cpu.read_flag_c(),cpu.read_flag_h(),cpu.read_flag_n(),cpu. read_flag_z());
}

void GBDebug::step(const std::vector<CommandArg> &args)
{
    UNUSED(args);
    print_console("{:4x}: {}\n",gb.cpu.read_pc(),gb.disass.disass_op(gb.cpu.read_pc()));
    gb.cpu.exec_instr_no_debug();
}

void GBDebug::disass(const std::vector<CommandArg> &args)
{
    if(!args.size())
    {
        print_console("usage: disass <addr> . <ammount>\n");
        return;
    }

    uint16_t addr;

    if(args[0].type == arg_type::integer)
    {
        addr = convert_imm(args[0].literal);
    }

    else
    {
        print_console("usage: disass <addr> . <ammount>\n");
        return;
    }
    

    if(args.size() == 1)
    {
        print_console("{:4x}: {}\n",addr,gb.disass.disass_op(addr));
    }

    else
    {
        int n;
        if(args[1].type == arg_type::integer)
        {
            n = convert_imm(args[1].literal);
        }

        else
        {
            print_console("usage: disass <addr> . <ammount>\n");
            return;
        }

        for(int i = 0; i < n; i++)
        {
            print_console("{:4x}: {}\n",addr,gb.disass.disass_op(addr));
            addr = (addr + gb.disass.get_op_sz(addr)) & 0xffff;
        }
    }
}


void GBDebug::print_mem(const std::vector<CommandArg> &args)
{
    if(!args.size())
    {
        print_console("usage: mem <addr> . <ammount>\n");
        return;
    }

    uint16_t addr;

    if(args[0].type == arg_type::integer)
    {
        addr = convert_imm(args[0].literal);
    }

    else
    {
        print_console("usage: mem <addr> . <ammount>\n");
        return;
    }
    

    if(args.size() == 1)
    {
        print_console("{:2x}: {}\n",addr,gb.mem.raw_read(addr));
    }

    else
    {
        int n;
        if(args[1].type == arg_type::integer)
        {
            n = convert_imm(args[1].literal);
        }

        else
        {
            print_console("usage: mem <addr> . <ammount>\n");
            return;
        }

        print_console("    ");

		for(int i = 0; i < 16; i++)
		{
			print_console("  {:2x}",i);
		}
		
		print_console("\n\n{:4x}: {:2x} ,",addr,gb.mem.raw_read(addr));
		for(int i = 1; i < n; i++)
		{	
			// makes it "slow" to format but oh well
			if(i % 16 == 0)
			{
				print_console("\n{:4x}: ",addr+i);
			}
			
			
			print_console("{:2x} ,",gb.mem.raw_read(addr+i));
			
		}
		
		print_console("\n");
    }
}

void GBDebug::print_trace(const std::vector<CommandArg> &args)
{
    UNUSED(args);
    print_console(trace.print());
}

}