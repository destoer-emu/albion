#project config
cmake_minimum_required(VERSION 3.10)

project(destoer-emu)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_definitions(-DFRONTEND_IMGUI -DIMGUI_IMPL_OPENGL_LOADER_GLEW)



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


# main source files
file(GLOB src_files
	"src/*.h"
	"src/*.cpp"
	"src/headers/*.h"
	"src/fmt/*.cpp"
	"src/fmt/*.h"
	"src/imgui/*.cpp"
	"src/imgui/*.h"
)

add_executable(destoer-emu ${src_files})

