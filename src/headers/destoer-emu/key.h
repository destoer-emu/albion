#include <frontend/qt/qt_window.h>
#include <frontend/sdl/sdl_window.h>
#include <frontend/imgui/imgui_window.h>


#ifdef FRONTEND_IMGUI
enum class emu_key
{
    enter = GLFW_KEY_ENTER,		
    space = GLFW_KEY_SPACE,
    down = GLFW_KEY_DOWN,
    up = GLFW_KEY_UP,
    left = GLFW_KEY_LEFT,
    right = GLFW_KEY_RIGHT,
    a = GLFW_KEY_A,
    s = GLFW_KEY_S,
    d = GLFW_KEY_D,
    f = GLFW_KEY_F,
    plus = GLFW_KEY_KP_ADD,
    minus = GLFW_KEY_KP_SUBTRACT
};
#endif


#ifdef FRONTEND_QT
enum class emu_key
{
    enter = Qt::Key_Return,		
    space = Qt::Key_Space,
    down = Qt::Key_Down,
    up = Qt::Key_Up,
    left = Qt::Key_Left,
    right = Qt::Key_Right,
    a = Qt::Key_A,
    s = Qt::Key_S,
    d = Qt::Key_D,
    f = Qt::Key_F,
    plus = Qt::Key_Plus,
    minus = Qt::Key_Minus
};
#endif


#ifdef FRONTEND_SDL
enum class emu_key
{
    enter = SDLK_RETURN,		
    space = SDLK_SPACE,
    down = SDLK_DOWN,
    up = SDLK_UP,
    left = SDLK_LEFT,
    right = SDLK_RIGHT,
    a = SDLK_a,
    s = SDLK_s,
    d = SDLK_d,
    f = SDLK_f,
    plus = SDLK_KP_PLUS,
    minus = SDLK_KP_MINUS
};
#endif

#ifdef FRONTEND_HEADLESS
enum class emu_key
{
    enter,		
    space,
    up,
    down,
    left,
    right,
    a,
    s,
    d,
    f,
    plus,
    minus
};
#endif