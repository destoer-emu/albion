
#include "imgui_window.h"
#include <destoer-emu/destoer-emu.h>


std::unordered_map<uint32_t,Breakpoint> &ImguiMainWindow::get_breakpoint_ref()
{
    switch(running_type)
    {
        case emu_type::gameboy: return gb.debug.breakpoints;
        case emu_type::gba: return gba.debug.breakpoints;
        case emu_type::none: assert(false);
    }
    assert(false);
}

void ImguiMainWindow::draw_breakpoints()
{

	static char input_breakpoint[12] = "";
    static bool break_r = false;
    static bool break_x = false;
    static bool break_w = false;
    static bool enabled[3] = {false};
    static int selected = -1;
    static uint32_t addr_selected;
	ImGui::Begin("Breakpoints");


	ImGui::Checkbox("read", &break_r);
	ImGui::SameLine(); ImGui::Checkbox("write", &break_w);
	ImGui::SameLine(); ImGui::Checkbox("execute", &break_x);


    
    const int type_idx = static_cast<int>(running_type);

    const bool old = enabled[type_idx];

    ImGui::SameLine(); ImGui::Checkbox("enable_all",&enabled[type_idx]);

    if(old != enabled[type_idx])
    {
        switch(running_type)
        {
            case emu_type::gameboy: gb.change_breakpoint_enable(enabled); break;
            case emu_type::gba: gba.change_breakpoint_enable(enabled); break;
            case emu_type::none: break;
        }
    }
    


	ImGui::InputText("", input_breakpoint, IM_ARRAYSIZE(input_breakpoint));


	// set breakpoints
	ImGui::SameLine();

	if (ImGui::Button("Set"))
	{
		if (is_valid_hex_string(input_breakpoint))
		{
			uint32_t breakpoint = strtoll(input_breakpoint, NULL, 16);

            switch(running_type)
            {
                case emu_type::gameboy: gb.debug.set_breakpoint(breakpoint,break_r,break_w,break_x,false,false,false); break;
                case emu_type::gba: gba.debug.set_breakpoint(breakpoint,break_r,break_w,break_x,false,false,false); break;
                case emu_type::none: break;
            }
			*input_breakpoint = '\0';
		}
	}


    auto &breakpoints = get_breakpoint_ref();

    if(ImGui::Button("disable"))
    {
        if(selected != -1)
        {
            breakpoints[addr_selected].disable();
        }
    }

    ImGui::SameLine();

    if(ImGui::Button("enable"))
    {
        if(selected != -1)
        {
            breakpoints[addr_selected].enable();
        }
    }

    ImGui::SameLine();

    if(ImGui::Button("delete"))
    {
        if(selected != -1)
        {
            breakpoints.erase(addr_selected);
            selected = -1; // gone from list so deselect it
        }
    }

	ImGui::Separator();

    ImGui::BeginChild("breakpoint list");

    // print breakpoints here
    int i = 0;
    for(auto &it : breakpoints)
    {
        i++;
        Breakpoint b = it.second;    
    
        std::string break_str = fmt::format(
            "{:04x}: {}{}{} {} {:x} {}",b.addr,
                b.break_setting & static_cast<int>(break_type::read)? "r" : "",
                b.break_setting & static_cast<int>(break_type::write)? "w" : "",
                b.break_setting & static_cast<int>(break_type::execute)? "x" : "",
                b.break_enabled? "enabled" : "disabled",
                b.value,
                b.value_enabled? "enabled" : "disabled"
        );
        if(ImGui::Selectable(break_str.c_str(),selected == i))
        {
            selected = i;
            // save the addr so we can re index the map later
            addr_selected = b.addr; 
        }
    }

    ImGui::EndChild();

	ImGui::End();	    
}


uint8_t ImguiMainWindow::read_mem(uint32_t addr)
{
    switch(running_type)
    {
        case emu_type::gba: return gba.mem.read_mem<uint8_t>(addr);
        case emu_type::gameboy: return gb.mem.raw_read(addr);
        case emu_type::none: return 0;
    }
    assert(false);
}


void ImguiMainWindow::write_mem(uint32_t addr,uint8_t v)
{
    switch(running_type)
    {
        case emu_type::gba: gba.mem.write_mem<uint8_t>(addr,v); break;
        case emu_type::gameboy: gb.mem.raw_write(addr,v); break;
        case emu_type::none: break;
    }
    assert(false);
}

// TODO: impl predefined ranges on this
void ImguiMainWindow::draw_memory()
{
    static uint32_t addr = 0x0;
    static uint32_t base_addr = 0;
    static uint32_t edit_addr = 0;
    static uint32_t edit_value = 0;
    static bool update = true;
    static const uint32_t CLIPPER_COUNT_TABLE[] = { 0x10000 / 0x10, 1024 * 1024, 0};
    static const uint32_t MAX_ADDR_TABLE[] = {0xffff,0x0E010000,0xdeadbeef};
    ImGui::Begin("memory-editor");

    const uint32_t MAX_ADDR = MAX_ADDR_TABLE[static_cast<int>(running_type)];
    const uint32_t CLIPPER_COUNT =  CLIPPER_COUNT_TABLE[static_cast<int>(running_type)];

	static char input_mem[12] = "";
	if (ImGui::Button("Goto"))
	{
		if (is_valid_hex_string(input_mem))
		{
            const uint32_t CLIPPER_ADDR_OFFSET = (CLIPPER_COUNT * 0x10) / 2;
			addr = strtoll(input_mem, NULL, 16) % MAX_ADDR;
            addr &= 0xfffffff0; // round to nearest section

            // if addr underflows set to 0
            if (addr - CLIPPER_ADDR_OFFSET > addr)
            {
                base_addr = 0;
            }

            
            else
            {
                base_addr = addr - CLIPPER_ADDR_OFFSET;
            }            


            printf("goto %x\n",base_addr);

            update = true;
			*input_mem = '\0';
		}  
	}

    ImGui::SameLine();

    ImGui::InputText("addr", input_mem, IM_ARRAYSIZE(input_mem));



	static char input_edit[12] = "";
	if (ImGui::Button("edit"))
	{
		if (is_valid_hex_string(input_mem) && is_valid_hex_string(input_edit))
		{
			edit_addr = strtoll(input_mem, NULL, 16);
            edit_value = strtoll(input_edit,NULL,16) & 0xff;
            *input_edit = '\0';
			*input_mem = '\0';
            write_mem(edit_addr,edit_value);
		}  
	}

    ImGui::SameLine();

    ImGui::InputText("value", input_edit, IM_ARRAYSIZE(input_edit));

    

    // padding
    ImGui::Text("          "); ImGui::SameLine();

    // draw col
    for(int i = 0; i < 0x10; i++)
    {
        ImGui::Text("%02x ",i);
        ImGui::SameLine();
    }

    ImGui::Text("\n");
    ImGui::Separator();


    ImGui::BeginChild("Memory View");
    ImGuiListClipper clipper(CLIPPER_COUNT); 

    float line_size = ImGui::GetTextLineHeightWithSpacing();

    if(update)
    {
        printf("update: %f\n",((addr-base_addr) / 0x10) * line_size);
        update = false;
        ImGui::SetScrollY(((addr-base_addr) / 0x10) * line_size);
    }

    while (clipper.Step())
    {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
        {
            
            ImGui::Text("%08x: ",(base_addr+i*0x10) % MAX_ADDR);
            ImGui::SameLine();


            for(int j = 0; j < 0x10; j++)
            {
                uint32_t dest = (base_addr+j+(i*0x10) % MAX_ADDR);
                ImGui::Text("%02x ",read_mem(dest));
                ImGui::SameLine();
            }
            ImGui::Text("\n");
        }
    }

    ImGui::EndChild();
    ImGui::End();
}