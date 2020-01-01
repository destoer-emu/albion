#project config
cmake_minimum_required(VERSION 3.10)

project(destoer-emu)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Werror -Wall -Wextra -pthread")


add_definitions(-DFRONTEND_IMGUI -DIMGUI_IMPL_OPENGL_LOADER_GLEW -DDEBUG -DLOG_CONSOLE)


if (WIN32)
  
# external libs 
list(APPEND CMAKE_PREFIX_PATH "C:/glew-2.1.0")
list(APPEND CMAKE_PREFIX_PATH "C:/SDL2-2.0.10")

#why does this not work nicely?
find_package(GLEW REQUIRED)
if (GLEW_FOUND)
    include_directories(${GLEW_INCLUDE_DIRS})
    #link_libraries(${GLEW_LIBRARIES})
	link_libraries("C:/glew-2.1.0/lib/Release/x64/glew32.lib" "C:/glew-2.1.0/lib/Release/x64/glew32s.lib")
endif()

find_package(OPENGL REQUIRED)
if (OPENGL_FOUND)
    include_directories(${OPENGL_INCLUDE_DIRS})
    link_libraries(${OPENGL_LIBRARIES})
endif()

#this could probably be better done
include_directories("C:/glfw/include")
link_libraries("C:/glfw/lib-vc2010-64/glfw3.lib")


# sdl
find_package(SDL2 REQUIRED)
if(SDL2_FOUND)
	include_directories(${SDL2_INCLUDE_DIRS})
	#link_libraries(${SDL2_LIBRARIES})
	link_libraries("C:/SDL2-2.0.10/lib/x64/SDL2.lib")
endif()

else()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lglfw  -lGL -lSDL2 -lGLEW")
endif()

# main source files
file(GLOB src_files
	"src/*.h"
	"src/*.cpp"
	"src/headers/*.h"
	"src/lib/*.h"
	"src/lib/*.cpp"
	"src/lib/fmt/*.cpp"
	"src/lib/fmt/*.h"
	"src/lib/imgui/*.cpp"
	"src/lib/imgui/*.h"
        "src/cpu/*.h"
        "src/cpu/*.cpp"
        "src/ppu/*.h"
        "src/ppu/*.cpp"
        "src/debug/*.h"
        "src/debug/*.cpp"
        "src/apu/*.h"
        "src/apu/*.cpp"
        "src/memory/*.h"
        "src/memory/*.cpp"
	"src/frontend/sdl/*.h"
        "src/frontend/sdl/*.cpp"
	"src/frontend/qt/*.h"
        "src/frontend/qt/*.cpp"
	"src/frontend/imgui/*.h"
        "src/frontend/imgui/*.cpp"

)

add_executable(destoer-emu ${src_files})

