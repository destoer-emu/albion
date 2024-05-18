#include "imgui_window.h"

class Texture
{
public:
    void init_texture(const int X, const int Y);
    void update_texture();
    void swap_buffer(std::vector<uint32_t> &other);
    GLuint get_texture() const;
    void draw_texture(u32 width_offset, u32 height_offset,u32 factor_x, u32 factor_y);
    int get_width() const { return x; } 
    int get_height() const { return y; } 
    bool valid() const { return is_valid; }

    std::vector<uint32_t> buf;

private:
    int x;
    int y;
    GLuint texture;
    u32 shader_program;
    bool first_time = true;
    bool is_valid = false;
};