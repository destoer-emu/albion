#project config

cmake_minimum_required(VERSION 3.10)

project(destoer-emu)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


#set(FRONTEND "QT")
#set(FRONTEND "SDL")
set(FRONTEND "IMGUI")


add_definitions(-DSDL_REQUIRED -DAUDIO_SDL -DCONTROLLER_SDL)
#add_definitions(-DBOUNDS_CHECK)
if(${FRONTEND} STREQUAL "IMGUI")
	add_definitions(-DFRONTEND_IMGUI -DIMGUI_IMPL_OPENGL_LOADER_GLEW -DDEBUG)
endif()

if(${FRONTEND} STREQUAL "SDL")
	add_definitions(-DFRONTEND_SDL)
endif()

if(${FRONTEND} STREQUAL "QT") # not supported yet come back to this
	add_definitions(-DFRONTEND_QT)
endif()


# make these paths less of a pain to modify :)

if(WIN32)
	set(SDL2_DIR "C:/SDL2-2.0.10")

	if(${FRONTEND} STREQUAL "IMGUI")  # needed only for imgui 
		# external libs 
		list(APPEND CMAKE_PREFIX_PATH "C:/glew-2.1.0")

		#why does this not work nicely?
		#find_package(GLEW REQUIRED)
		#if (GLEW_FOUND)
			#include_directories(${GLEW_INCLUDE_DIRS})
			#link_libraries(${GLEW_LIBRARIES})
			include_directories("C:/glew-2.1.0/include/")
			link_libraries("C:/glew-2.1.0/lib/Release/x64/glew32.lib" "C:/glew-2.1.0/lib/Release/x64/glew32s.lib")
		#endif()


		find_package(OPENGL REQUIRED)
		if (OPENGL_FOUND)
			include_directories(${OPENGL_INCLUDE_DIRS})
			link_libraries(${OPENGL_LIBRARIES})
		endif()

		#this doesent work either :P
		#find_package(GLFW REQUIRED)
		#if(GLFW_FOUND)
			#include_directories(${GLFW_INCLUDE_DIRS})
			#include_directories(${GLFW_LIBRARIES})
			include_directories("C:/glfw-3.3.2.bin.WIN64/include")
			link_libraries("C:/glfw-3.3.2.bin.WIN64/lib-vc2019/glfw3.lib")
		#endif()
	endif()


	# sdl allways needed
	#find_package(SDL2 REQUIRED)
	#if(SDL2_FOUND)
		#include_directories(${SDL2_INCLUDE_DIRS})
		#link_libraries(${SDL2_LIBRARIES})
		# if no sdl2 
		include_directories("C:/SDL2-2.0.12/include")
		link_libraries("C:/SDL2-2.0.12/lib/x64/SDL2.lib")
	#endif()
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Werror -Wall -Wextra -pthread -Ofast")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lSDL2")
	if(${FRONTEND} STREQUAL "IMGUI") 
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lglfw  -lGL -lGLEW")
	endif()
endif()



if(${FRONTEND} STREQUAL "QT") 
	if(WIN32)
		list(APPEND CMAKE_PREFIX_PATH "C:/Qt/5.9.9/msvc2017_64")
		find_package(OPENGL REQUIRED)
		if (OPENGL_FOUND)
			include_directories(${OPENGL_INCLUDE_DIRS})
			link_libraries(${OPENGL_LIBRARIES})
		endif()
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lGL")
	endif()

	set(CMAKE_AUTOMOC ON)
	set(CMAKE_AUTORCC ON)
	set(CMAKE_AUTOUIC ON)
	find_package(Qt5 COMPONENTS Widgets OpenGL REQUIRED)
	link_libraries(Qt5::Widgets)
	link_libraries(Qt5::OpenGL)
endif()



# main source files
file(GLOB src_files
	"src/*.h"
	"src/*.cpp"
	"src/common/destoer-emu/*.cpp"
	"src/headers/destoer-emu/*.h"
	"src/common/thirdparty/fmt/src/*.cc"
	"src/common/thirdparty/fmt/include/*.h"	
    "src/gb/*.cpp"	
    "src/gb/cpu/*.cpp"
    "src/gb/apu/*.cpp"
	"src/gb/ppu/*.cpp"
    "src/gb/memory/*.cpp"
	"src/gb/headers/gb/*.h"
	"src/gba/*.cpp"
	"src/gba/cpu/*.cpp"
	"src/gba/memory/*.cpp"
	"src/gba/ppu/*.cpp"
	"src/gba/apu/*.cpp"
	"src/gba/headers/gba/*.h"
	"src/frontend/gb/*.cpp"
	"src/frontend/gb/*.h"
	"src/frontend/gba/*.cpp"
	"src/frontend/gba/*.h"
)

# add some includes
include_directories("src/headers")
include_directories("src/common/thirdparty/fmt/include")
include_directories("src/common/thirdparty")
include_directories("src/common/thirdparty/imgui")
include_directories("src")


if(${FRONTEND} STREQUAL "IMGUI")
	file(GLOB frontend_files
		"src/frontend/imgui/*.h"
		"src/frontend/imgui/*.cpp"
		"src/common/thirdparty/imgui/*.cpp"
		"src/common/thirdparty/imgui/*.h"
		"src/common/thirdparty/imgui/examples/imgui_impl_glfw.cpp"
		"src/common/thirdparty/imgui/examples/imgui_impl_opengl3.cpp"	
	)
endif()

if(${FRONTEND} STREQUAL "QT")
	file(GLOB frontend_files
		"src/frontend/qt/*.h"
		"src/frontend/qt/*.cpp"
	)
endif()

if(${FRONTEND} STREQUAL "SDL")
	file(GLOB frontend_files
		"src/frontend/sdl/*.h"
		"src/frontend/sdl/*.cpp"
	)
endif()

if(${FRONTEND} STREQUAL "QT")
	if(WIN32)
		add_executable(destoer-emu  WIN32 ${src_files} ${frontend_files})
	else()
		add_executable(destoer-emu  ${src_files} ${frontend_files})
	endif()
else()
	add_executable(destoer-emu ${src_files} ${frontend_files})
endif()
