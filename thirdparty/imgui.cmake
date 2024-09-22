FetchContent_Declare(
  imgui
  GIT_REPOSITORY "https://github.com/ocornut/imgui.git"
  GIT_TAG a9f72ab6818c3e55544378aa44c7659de7e5510f
)

FetchContent_GetProperties(imgui)
if (NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif ()

add_library(
    imgui
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})

if(WIN32)
    find_package(OPENGL REQUIRED)
    target_link_libraries(imgui PUBLIC SDL2::SDL2-static libglew_static OpenGL::GL)
else()
    target_link_libraries(imgui PUBLIC SDL2::SDL2-static libglew_static GL)
endif()

FetchContent_MakeAvailable(imgui)