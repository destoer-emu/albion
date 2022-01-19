#include <destoer-emu/debug.h>

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

bool Debug::breakpoint_hit(uint64_t addr, uint64_t value, break_type type)
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

    const bool hit =  b.is_hit(type,value);


    if(hit)
    {

        if(breakpoints_enabled && !b.watch)
        {
            print_console("[{:x}]:{} breakpoint hit at {:x}:{:x}\n",get_pc(),static_cast<int>(type),addr,value);
        }

        // this is a watchpoint we just want to print to the console
        // with some debug info 
        else if(watchpoints_enabled && b.watch)
        {
            //print_watchpoint(b);
            print_console("[{:x}]:{} watch hit at {:x}:{:x}\n",get_pc(),static_cast<int>(type),addr,value);
            return false;
        }
    }

    return hit && breakpoints_enabled;
}

void Debug::set_breakpoint(uint64_t addr,bool r, bool w, bool x, bool value_enabled, uint64_t value, bool watch)
{
    Breakpoint b;

    b.set(addr,r,w,x,value_enabled,value,true,watch);

    breakpoints[addr] = b;
}


void Breakpoint::set(uint64_t addr, bool r, bool w, bool x, 
    bool value_enabled,uint64_t value,bool break_enabled, bool watch)
{
    this->value = value;
    this->addr = addr;
    this->break_enabled = break_enabled;
    this->value_enabled = value_enabled;
    this->watch = watch;
    this->break_setting = 0;

    if(r)
    {
        break_setting |= static_cast<int>(break_type::read);
    }

    if(w)
    {
        break_setting |= static_cast<int>(break_type::write);
    }

    if(x)
    {
        break_setting |= static_cast<int>(break_type::execute);
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

bool Breakpoint::is_hit(break_type type,uint64_t value)
{
    // if the its not enabled or the value does not match if enabled
    // then it is not hit
    if(!break_enabled || (value_enabled && this->value != value))
    {
        return false;
    }

    // if the type the breakpoint has been triggered for
    // is not being watched then we aernt interested
    if((static_cast<int>(type) & break_setting) == 0)
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
    ImGui::BeginChild("console_child",ImVec2(0, 300), true);

    static bool update = false;

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

    if(ImGui::InputText("", input, IM_ARRAYSIZE(input),ImGuiInputTextFlags_EnterReturnsTrue))
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

    u32 addr;

    if(args[1].type == token_type::integer)
    {
        addr = convert_imm(args[1].literal);
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
        if(args[2].type == token_type::integer)
        {
            n = convert_imm(args[2].literal);
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
            b.break_setting & static_cast<int>(break_type::read)? "r" : "",
            b.break_setting & static_cast<int>(break_type::write)? "w" : "",
            b.break_setting & static_cast<int>(break_type::execute)? "x" : "",
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

    uint64_t addr;

    if(args[1].type == token_type::integer)
    {
        addr = convert_imm(args[1].literal);
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
        if(args[2].type == token_type::integer)
        {
            n = convert_imm(args[2].literal);
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


void Debug::set_break_internal(const std::vector<Token> &args, bool watch)
{

    if(args.size() <= 1 || args.size() > 4)
    {
        print_console("usage: {} <addr> . <value> . <type> . \n",watch? "watch" : "break",args.size());
        return;
    }

    if(args[1].type != token_type::integer)
    {
        print_console("expected int got string: {}\n",args[1].literal);
        return;
    }

    const auto addr = convert_imm(args[1].literal);

    bool r = false;
    bool w = false;
    bool x = true;
    bool value_enabled = false;

    auto value = 0xdeadbeef;

    // for 2nd arg allow tpye or value
    if(args.size() >= 3)
    {
        if(args[2].type == token_type::integer)
        {
            value = convert_imm(args[2].literal);
            value_enabled = true;
        }

        // type set
        else
        {
            for(const auto c: args[2].literal)
            {
                switch(c)
                {
                    case 'r': r = true; break;
                    case 'w': w = true; break;
                    case 'x': x = true; break;

                    default:
                    {
                        print_console("expected type (rwx) got: {}\n",args[2].literal);
                        return;
                    }
                }
            }

            // optinal value after the type
            if(args.size() == 4)
            {
                // last is value and previous was type set
                if(args[3].type == token_type::integer)
                {
                    value = convert_imm(args[3].literal);
                    value_enabled = true;
                }

                else
                {
                    print_console("expected int got string: {}\n",args[3].literal);
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

void Debug::debug_input()
{
    print_console("{:16x}\n",get_pc());
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
        
        execute_command(args);
        std::cin.clear();
    }
}  

#endif