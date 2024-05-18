#include <albion/debug.h>


token_type read_type(const Token& token)
{
    return token_type(token.index());
} 

u64 read_u64(const Token& token)
{
    return std::get<u64>(token);
}

std::string read_str(const Token& token)
{
    return std::get<std::string>(token);
}

// wake the instance up
void Debug::wake_up()
{
    halted = false;
}


#ifdef DEBUG

// TODO: add memory searching


// halt the instance so the debugger is free to poke at it
void Debug::halt()
{
    halted = true;
}

//disable any form of breakpoint
void Debug::disable_everything()
{
    wake_up();
    breakpoints_enabled = false;
}

b32 Debug::breakpoint_hit_internal(u64 addr, u64 value, break_type type)
{
    if(!breakpoints_enabled && !watchpoints_enabled)
    {
        return false;
    }

    // map does not have a matching breakpoint
    if(!breakpoints.count(addr))
    {
        return false;
    }

    auto b = breakpoints[addr];

    const b32 hit =  b.is_hit(type,value);


    if(hit)
    {

        if(breakpoints_enabled && !b.watch)
        {
            print_console("[{:x}]:{} breakpoint hit at {:x}:{:x}\n",read_pc(),u32(type),addr,value);
        }

        // this is a watchpoint we just want to print to the console
        // with some debug info 
        else if(watchpoints_enabled && b.watch)
        {
            //print_watchpoint(b);
            print_console("[{:x}]:{} watch hit at {:x}:{:x}\n",read_pc(),u32(type),addr,value);
            return false;
        }
    }

    return hit && breakpoints_enabled;
}

void Debug::set_breakpoint(u64 addr,b32 r, b32 w, b32 x, b32 value_enabled, u64 value, b32 watch)
{
    Breakpoint b;

    b.set(addr,r,w,x,value_enabled,value,true,watch);

    breakpoints[addr] = b;
}


void Breakpoint::set(u64 addr, b32 r, b32 w, b32 x, 
    b32 value_enabled,u64 value,b32 break_enabled, b32 watch)
{
    this->value = value;
    this->addr = addr;
    this->break_enabled = break_enabled;
    this->value_enabled = value_enabled;
    this->watch = watch;
    this->break_setting = 0;

    if(r)
    {
        break_setting |= u32(break_type::read);
    }

    if(w)
    {
        break_setting |= u32(break_type::write);
    }

    if(x)
    {
        break_setting |= u32(break_type::execute);
    }

}

void Breakpoint::disable()
{
    break_enabled = false;
}

void Breakpoint::enable()
{
    break_enabled = true;
}

b32 Breakpoint::is_hit(break_type type,u64 value)
{
    // if the its not enabled or the value does not match if enabled
    // then it is not hit
    if(!break_enabled || (value_enabled && this->value != value))
    {
        return false;
    }

    // if the type the breakpoint has been triggered for
    // is not being watched then we aernt interested
    if((u32(type) & break_setting) == 0)
    {
        return false;
    }


    return true;
}


Debug::Debug()
{
    log_file.open("emu.log");
    if(!log_file)
    {
        puts("failed to open log file!");
        exit(1);
    }
    disable_everything();

#ifdef FRONTEND_IMGUI
    console.resize(256);
    assert((console.size() & (console.size() - 1)) == 0);
#endif
}

Debug::~Debug()
{
    log_file.close();
}

#ifdef FRONTEND_IMGUI
#include <frontend/imgui/imgui_window.h>

void Debug::draw_console()
{
    ImGui::BeginChild("console_child",ImVec2(0, 350), true);

    static b32 update = false;

    for (size_t i = 0; i < console.size(); i++)
    {         
        const auto str = console[(i+console_idx) & (console.size() - 1)];
        ImGui::Text("%s",str.c_str());
    }
    
    if(update)
    {
        update = false;
        ImGui::SetScrollHereY(1.0f);
    }



    ImGui::EndChild();

    static char input[128] = "";

    if(ImGui::InputText("##console-text-input", input, IM_ARRAYSIZE(input),ImGuiInputTextFlags_EnterReturnsTrue))
    {
        if(*input)
        {
            print_console("$ {}\n",input);
            std::vector<Token> args;
            if(!tokenize(input,args))
            {
                // TODO: provide better error reporting
                print_console("one or more args is invalid");
            }
            
            execute_command(args);
            *input = '\0';
        }
        // keep in text box after input
        ImGui::SetKeyboardFocusHere(-1);

        // scroll to bottom after command
        update = true;
    }
}

#endif

// console impl


void Debug::print_mem(const std::vector<Token> &args)
{
    if(args.size() <= 1)
    {
        print_console("usage: mem <addr> . <ammount>\n");
        return;
    }

    u64 addr;

    if(read_type(args[1]) == token_type::u64_t)
    {
        addr = read_u64(args[1]);
    }

    else
    {
        print_console("usage: mem <addr> . <ammount>\n");
        return;
    }
    

    if(args.size() == 2)
    {
        print_console("{:8x}: {}\n",addr,read_mem(addr));
    }

    else
    {
        int n;
        if(read_type(args[2]) == token_type::u64_t)
        {
            n = read_u64(args[2]);
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
		
		print_console("\n\n{:8x}: {:2x} ,",addr,read_mem(addr));
		for(int i = 1; i < n; i++)
		{	
			// makes it "slow" to format but oh well
			if(i % 16 == 0)
			{
				print_console("\n{:8x}: ",addr+i);
			}
			
			
			print_console("{:2x} ,",read_mem(addr+i));
			
		}
		
		print_console("\n");
    }
}

void Debug::print_trace(const std::vector<Token> &args)
{
    UNUSED(args);
    print_console(trace.print());
}

// TODO: add optional arg to change individual settings on invidual breakpoints
void Debug::clear_breakpoint(const std::vector<Token> &args)
{
    UNUSED(args);
    breakpoints.clear();
    print_console("breakpoints cleared\n");
}


void Debug::enable_breakpoint(const std::vector<Token> &args)
{
    UNUSED(args);
    breakpoints_enabled = true;
    if(!watchpoints_enabled)
    {
        change_breakpoint_enable(true);
    }
    print_console("breakpoints enabled\n");
}

void Debug::disable_breakpoint(const std::vector<Token> &args)
{
    UNUSED(args);
    breakpoints_enabled = false;
    // if we dont need any kind of break/watch then we can shortcut checks in the gb memory handler for perf
    if(!watchpoints_enabled)
    {
        change_breakpoint_enable(false);
    }
    print_console("breakpoints disabled\n");
}

void Debug::enable_watch(const std::vector<Token> &args)
{
    UNUSED(args);
    watchpoints_enabled = true;
    if(!breakpoints_enabled)
    {
        change_breakpoint_enable(true);
    }
    print_console("watchpoints enabled\n");
}

void Debug::disable_watch(const std::vector<Token> &args)
{
    UNUSED(args);
    watchpoints_enabled = false;
    if(!breakpoints_enabled)
    {
        change_breakpoint_enable(false);
    }
    print_console("watchpoints disabled\n");
}



void Debug::print_breakpoint(const Breakpoint &b)
{
    print_console(
        "{:04x}: {}{}{} {} {:x} {}\n",b.addr,
            b.break_setting & u32(break_type::read)? "r" : "",
            b.break_setting & u32(break_type::write)? "w" : "",
            b.break_setting & u32(break_type::execute)? "x" : "",
            b.break_enabled? "enabled" : "disabled",
            b.value,
            b.value_enabled? "enabled" : "disabled"
    );    
}

void Debug::list_breakpoint(const std::vector<Token> &args)
{
    UNUSED(args);
    for(const auto &it: breakpoints)
    {
        const auto b = it.second;
        if(!b.watch)
        {
            print_breakpoint(b);
        }
    }
}

void Debug::list_watchpoint(const std::vector<Token> &args)
{
    UNUSED(args);
    for(const auto &it: breakpoints)
    {
        const auto b = it.second;
        if(b.watch)
        {
            print_breakpoint(b);
        }
    }
}


void Debug::disass_internal(const std::vector<Token> &args)
{
    if(args.size() <= 1)
    {
        print_console("usage: disass <addr> . <ammount>\n");
        return;
    }

    u64 addr;

    if(read_type(args[1]) == token_type::u64_t)
    {
        addr = read_u64(args[1]);
    }

    else
    {
        print_console("usage: disass <addr> . <ammount>\n");
        return;
    }
    

    if(args.size() == 2)
    {
        print_console("{}\n",disass_instr(addr));
    }

    else
    {
        int n;
        if(read_type(args[2]) == token_type::u64_t)
        {
            n = read_u64(args[2]);
        }

        else
        {
            print_console("usage: disass <addr> . <ammount>\n");
            return;
        }

        for(int i = 0; i < n; i++)
        {
            print_console("{}\n",disass_instr(addr));
            addr = (addr + get_instr_size(addr));
        }
    }    
}


void Debug::set_break_internal(const std::vector<Token> &args, b32 watch)
{

    if(args.size() <= 1 || args.size() > 4)
    {
        print_console("usage: {} <addr> . <value> . <type> . \n",watch? "watch" : "break",args.size());
        return;
    }

    if(read_type(args[1]) != token_type::u64_t)
    {
        print_console("expected int got string: {}\n",read_str(args[1]));
        return;
    }

    const auto addr = read_u64(args[1]);

    b32 r = false;
    b32 w = false;
    b32 x = true;
    b32 value_enabled = false;

    auto value = 0xdeadbeef;

    // for 2nd arg allow tpye or value
    if(args.size() >= 3)
    {
        if(read_type(args[2]) == token_type::u64_t)
        {
            value = read_u64(args[2]);
            value_enabled = true;
        }

        // type set
        else
        {
            const auto literal = read_str(args[2]);

            for(const auto c: literal)
            {
                switch(c)
                {
                    case 'r': r = true; break;
                    case 'w': w = true; break;
                    case 'x': x = true; break;

                    default:
                    {
                        print_console("expected type (rwx) got: {}\n",literal);
                        return;
                    }
                }
            }

            // optinal value after the type
            if(args.size() == 4)
            {
                // last is value and previous was type set
                if(read_type(args[3]) == token_type::u64_t)
                {
                    value = read_u64(args[3]);
                    value_enabled = true;
                }

                else
                {
                    print_console("expected int got string: {}\n",read_str(args[3]));
                    return;
                }
            }
        }
    }

    set_breakpoint(addr,r,w,x,value_enabled,value,watch);

    if(watch)
    {
        print_console("watchpoint set at: {:x}\n",addr);
        print_console("watchpoint enable: {}\n",watchpoints_enabled);        
    }

    else
    {
        print_console("breakpoint set at: {:x}\n",addr);
        print_console("breakpoint enable: {}\n",breakpoints_enabled);        
    }
}

void Debug::breakpoint(const std::vector<Token> &args)
{
    set_break_internal(args,false);
}


void Debug::watch(const std::vector<Token> &args)
{
    set_break_internal(args,true);
}

void Debug::run(const std::vector<Token> &args)
{
    UNUSED(args);
    wake_up();
    quit = true;
    print_console("resuming execution\n");
}


// basic tokenizer
template<typename F>
b32 verify_immediate_internal(const std::string &line, uint32_t &i, F lambda)
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


b32 verify_immediate(const std::string &line, std::string &literal)
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

    b32 valid = false;


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


u32 convert_imm(const std::string &imm)
{
    try 
    {
        if(imm.size() >= 3 && imm.substr(0,2) == "0b")
        {
            return u64(std::stoll(imm.substr(2),0,2));
        }

        // stoi wont auto detect base for binary strings?
        return u64(std::stoll(imm,0,0));
    }

    catch(std::exception &ex)
    {
        printf("stoi exception\n");
        std::cout << ex.what() << std::endl;
        exit(1);
    }
}

b32 decode_imm(const std::string &line, uint32_t &i,std::string &literal)
{
    const auto success = verify_immediate(line.substr(i),literal);

    // set one back for whatever the terminating character was
    i--;

    i += literal.size();  

    return success;
}

b32 Debug::tokenize(const std::string &line,std::vector<Token> &tokens)
{
    tokens.clear();
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

            // comment end of line
            case ';': return true;


         
            default:
            {   
                Token token;
                std::string literal = "";

                // integer
                if(isdigit(c))
                {
                    if(!decode_imm(line,i,literal))
                    {
                        return false;
                    }

                    token = convert_imm(literal);
                }

                // string
                else
                {
                    for(; i < line.size(); i++)
                    {
                        const auto c = line[i];
                        if(c == ' ')
                        {
                            break;
                        }

                        literal += c;
                    }

                    // parse a var
                    if(literal[0] == '$')
                    {
                        u64 value = 0;
                        if(!read_var(literal.substr(1),&value))
                        {
                            print_console("could not read var {}\n",literal);
                            return false;
                        }

                        token = value;
                    }

                    else
                    {
                        token = literal;
                    }
                }

                // push token
                tokens.push_back(token);
                break;
            }
        }
    }
    return true;
}


b32 Debug::invalid_command(const std::vector<Token>& args)
{
    // No args, we have nothing to process
    if(!args.size())
    {
        return true;
    }

    // We expect a string for this
    if(read_type(args[0]) != token_type::str_t)
    {
        return true;
    }

    return false;
}

void Debug::debug_input()
{
    print_console("break at {:16x}\n",read_pc());
    std::string line = "";


    std::vector<Token> args;
    quit = false;
    while(!quit)
    {
        print_console("$ ");
        std::getline(std::cin,line);

        // lex the line and pull the command name along with the args.
        if(!tokenize(line,args))
        {
            // TODO: provide better error reporting
            print_console("one or more args is invalid");
        }

        // TODO:
        //resolve_vars(args);
        
        execute_command(args);
        std::cin.clear();
    }
}  


u64 Debug::read_pc()
{
    u64 pc;
    read_var("pc",&pc);

    return pc;
}

#endif