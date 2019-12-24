
#ifdef FRONTEND_IMGUI

#include "headers/file_dialog.h"
#ifdef _WIN32
#include <windows.h>
// this gets a nice crash at the momement
std::string open_file_dialog(std::string path)
{
    OPENFILENAME ofn;
    ZeroMemory(&ofn,sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    char file_name[MAX_PATH] = "";
    ofn.lpstrFile = file_name;
    ofn.lpstrFilter = "All Files (*.*)\0*.*\0";
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST;
    ofn.lpstrDefExt = ""; 
    ofn.lpstrTitle = "Open rom...";
    ofn.lpstrInitialDir = path.c_str();

    if(GetOpenFileName(&ofn))
    {
        return std::string(file_name);
    }

    else
    {
        return "";
    }   
}

#else

std::string open_file(std::string path)
{
    static_assert(false,"unknown platform for open file!");
}

#endif

#endif // end frontend_guard
