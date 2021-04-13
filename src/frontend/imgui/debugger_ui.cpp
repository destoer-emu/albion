
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
            case emu_type::gameboy: gb.change_breakpoint_enable(enabled[type_idx]); break;
            case emu_type::gba: gba.change_breakpoint_enable(enabled[type_idx]); break;
            case emu_type::none: break;
        }
    }
    


	ImGui::InputText("", input_breakpoint, IM_ARRAYSIZE(input_breakpoint),ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);


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
}

struct MemRegion
{
    MemRegion(const char *n,uint32_t o, uint32_t s) : name(n), offset(o), size(s)
    {}

    const char *name;
    const uint32_t offset;
    const uint32_t size;
};


const uint32_t GAMEBOY_MEM_REGION_SIZE = 7;

MemRegion GAMEBOY_MEM_REGION_TABLE[GAMEBOY_MEM_REGION_SIZE] =
{
    {"rom 0000-7fff",0,0x8000},
    {"vram 8000-9fff",0x8000,0x2000},
    {"sram a000-bfff",0xa000,0x2000},
    {"wram c000-dfff",0xc000,0x2000},
    {"echo ram e000-fdff",0xe000,0x1e00},
    {"oam fe00-fe9f",0xfe00,0xa0},
    {"io ff00-ffff",0xff00,0x100},
};


const uint32_t GBA_MEM_REGION_SIZE = 9;

MemRegion GBA_MEM_REGION_TABLE[GBA_MEM_REGION_SIZE] =
{
    {"bios 00000000-00003fff",0,0x4000},
    {"onboard wram 02000000-0203ffff",0x02000000,0x40000},
    {"onchip wram 03000000-03007fff",0x03000000,0x8000},
    {"io 04000000-040003ff",0x43000000,0x400},
    {"obj ram 05000000-050003ff",0x05000000,0x400},
    {"vram 06000000-060017fff",0x06000000,0x18000},
    {"oam 07000000-070003ff",0x07000000,0x400},
    {"rom 08000000-09ffffff",0x08000000,32*1024*1024},
    {"sram 0e000000-0e00ffff",0x0e000000,0x10000},
};


// TODO: okay now we just figure out how to take the key input and this works perfectly
void ImguiMainWindow::draw_memory()
{
    if(running_type == emu_type::none)
    {
        return;
    }

    ImGui::Begin("memory-editor");



    static uint32_t region_idx_table[2] = {0};
    static const uint32_t SIZE_TABLE[2] = {GAMEBOY_MEM_REGION_SIZE,GBA_MEM_REGION_SIZE};
    static const MemRegion *MEM_REGION_PTR_TABLE[2] = {GAMEBOY_MEM_REGION_TABLE,GBA_MEM_REGION_TABLE};

    const auto type_idx = static_cast<uint32_t>(running_type);
    const auto region_ptr =  MEM_REGION_PTR_TABLE[type_idx];
    const auto size = SIZE_TABLE[type_idx];
    auto region_idx = region_idx_table[type_idx];

    const uint32_t base_addr = region_ptr[region_idx].offset;
    const uint32_t clipper_count = region_ptr[region_idx].size / 0x10;

    static int y = -1;
    static int x = -1;

    // combo box to select view type
    if(ImGui::BeginCombo("",region_ptr[region_idx].name))
    {
        for(uint32_t i = 0; i < size; i++)
        {
            if(ImGui::Selectable(region_ptr[i].name,region_idx == i))
            {
                region_idx = i;
                region_idx_table[type_idx] = i;
                // we have just changed reset to the top of the memory viewer
                ImGui::SetScrollY(0.0);
                x = -1;
                y = -1;
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    ImGui::Text("edit: %x",base_addr + (y * 0x10) + x); ImGui::SameLine();
    static char input[3] = {0};
    ImGui::PushItemWidth(20.0);
    if(ImGui::InputText(" ", input, IM_ARRAYSIZE(input),ImGuiInputTextFlags_EnterReturnsTrue | 
        ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase))
    {
        if(x != -1 && y != -1)
        {
            write_mem(base_addr + (y * 0x10) + x,strtol(input,NULL,16));
            *input = '\0';
            x += 1;
            if(x == 0x10)
            {
                y++;
                x = 0;
            }
            // keep in text box after input
            ImGui::SetKeyboardFocusHere(-1);
        }
    }
    ImGui::PopItemWidth();

    // draw col
    ImGui::Text("          "); ImGui::SameLine();

    ImGui::BeginTable("offsets",0x10, ImGuiTableFlags_SizingFixedFit);
    for(int i = 0; i < 0x10; i++)
    {
        ImGui::TableNextColumn();
        ImGui::Text("%02x ",i);
    }
    ImGui::EndTable();
    ImGui::Separator();

    ImGui::BeginChild("Memory View");
    ImGuiListClipper clipper(clipper_count); 


    while (clipper.Step())
    {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
        {
            ImGui::BeginTable("offsets",0x11, ImGuiTableFlags_SizingFixedFit);
            ImGui::TableNextColumn();
            ImGui::Text("%08x: ",(base_addr+i*0x10)); 
        

            for(int j = 0; j < 0x10; j++)
            {
                ImGui::TableNextColumn();
                uint32_t dest = (base_addr+j+(i*0x10));
                if(ImGui::Selectable(fmt::format("{:02x} ",read_mem(dest)).c_str(),i == y && j == x,ImGuiSelectableFlags_AllowDoubleClick))
                {
                    y = i;
                    x = j;
                }
            }
            ImGui::EndTable();
        }
    }


    ImGui::EndChild();
    ImGui::End();
}