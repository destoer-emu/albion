
#include "imgui_window.h"

void Texture::update_texture()
{
    glEnable(GL_TEXTURE_2D); 
    glBindTexture(GL_TEXTURE_2D,texture);
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,x,y,GL_RGBA, GL_UNSIGNED_BYTE,buf.data());
    glBindTexture(GL_TEXTURE_2D,0);
    glDisable(GL_TEXTURE_2D); 
}

void Texture::init_texture(const int X, const int Y)
{
    x = X;
    y = Y;
    buf.resize(x*y);
    std::fill(buf.begin(),buf.end(),0);

    glEnable(GL_TEXTURE_2D); 
    if(first_time)
    {
        glGenTextures(1,&texture);
        first_time = false;
    }
    glBindTexture(GL_TEXTURE_2D,texture);

    // setup our texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,x,y,0,GL_RGBA, GL_UNSIGNED_BYTE,buf.data());

    glBindTexture(GL_TEXTURE_2D,0);
    glDisable(GL_TEXTURE_2D); 
}

void Texture::swap_buffer(std::vector<uint32_t> &other)
{
    std::swap(other,buf);
}


void Texture::draw_texture(u32 width_offset, u32 height_offset,u32 factor_x, u32 factor_y)
{
    // render this straight to background with imgui
    ImVec2 min_pos = ImVec2(width_offset,height_offset);
    ImVec2 max_pos = ImVec2(width_offset + (x * factor_x), height_offset + (y * factor_y));

    auto bg = ImGui::GetBackgroundDrawList();
    bg->AddImage((void*)(intptr_t)texture,min_pos,max_pos, ImVec2(0, 0), ImVec2(1, 1));
}

GLuint Texture::get_texture() const
{
    return texture;
}