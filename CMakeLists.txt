#project config

cmake_minimum_required(VERSION 3.10)

project(destoer-emu)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


#set(FRONTEND "QT")
set(FRONTEND "SDL")
#set(FRONTEND "IMGUI")
#set(FRONTEND "DESTOER")

# external frontend
#set(FRONTEND "HEADLESS")


#add_definitions(-DBOUNDS_CHECK)
if(${FRONTEND} STREQUAL "IMGUI")
	add_definitions(-DAUDIO_ENABLE -DSDL_REQUIRED -DAUDIO_SDL -DCONTROLLER_SDL -DFETCH_SPEEDHACK)
	add_definitions(-DFRONTEND_IMGUI -DIMGUI_IMPL_OPENGL_LOADER_GLEW -DDEBUG -DLOG_CONSOLE)
endif()

if(${FRONTEND} STREQUAL "SDL")
	add_definitions(-DAUDIO_ENABLE -DSDL_REQUIRED -DAUDIO_SDL -DCONTROLLER_SDL -DDEBUG -DFETCH_SPEEDHACK)
	add_definitions(-DFRONTEND_SDL)
endif()


if(${FRONTEND} STREQUAL "DESTOER")
	add_definitions(-DAUDIO_ENABLE -DSDL_REQUIRED -DAUDIO_SDL -DCONTROLLER_SDL -DDEBUG -DFETCH_SPEEDHACK)
	add_definitions(-DFRONTEND_DESTOER)
endif()

if(${FRONTEND} STREQUAL "QT") # not supported yet come back to this
	add_definitions(-DAUDIO_ENABLE -DSDL_REQUIRED -DAUDIO_SDL -DCONTROLLER_SDL -DDEBUG -DFETCH_SPEEDHACK)
	add_definitions(-DFRONTEND_QT)
endif()

if(${FRONTEND} STREQUAL "HEADLESS")
	add_definitions(-DFRONTEND_HEADLESS)
endif()





# source files 

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
	"src/n64/n64.cpp"
	"src/n64/*.h"
	"src/frontend/gb/*.cpp"
	"src/frontend/gb/*.h"
	"src/frontend/gba/*.cpp"
	"src/frontend/gba/*.h"
	"src/frontend/*.cpp"
)

# add some includes
include_directories("src/headers")
include_directories("src/common/thirdparty/fmt/include")
include_directories("src/common/thirdparty")
include_directories("src/common/thirdparty/imgui")
include_directories("src")

#destoer lib
include_directories("destoer-lib/src/")
add_definitions(-DFORWARD_DEF_DESTOER_LIB)

if(${FRONTEND} STREQUAL "IMGUI")
	include_directories("/usr/include/SDL2")
	file(GLOB frontend_files
		"src/frontend/imgui/*.h"
		"src/frontend/imgui/*.cpp"
		"src/common/thirdparty/imgui/*.cpp"
		"src/common/thirdparty/imgui/*.h"
		"src/common/thirdparty/imgui/backends/imgui_impl_sdl.cpp"
		"src/common/thirdparty/imgui/backends/imgui_impl_opengl3.cpp"	
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

if(${FRONTEND} STREQUAL "DESTOER")
	file(GLOB frontend_files
		"src/frontend/destoer/*.h"
		"src/frontend/destoer/*.cpp"
	)

	include_directories("src/frontend/destoer")
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






# make these paths less of a pain to modify :)

if(WIN32)
	if(${FRONTEND} STREQUAL "IMGUI")  # needed only for imgui 
		# external libs

		# opengl
		find_package(OPENGL REQUIRED)
		target_link_libraries(destoer-emu OpenGL::GL)

		# glew
		list(APPEND CMAKE_PREFIX_PATH "C:/glew-2.1.0")
		find_package(GLEW REQUIRED)
		target_link_libraries(destoer-emu GLEW::GLEW)

		# for some reason building is broken and extra steps are required
		add_library(GLEW SHARED IMPORTED)
		set_target_properties(GLEW PROPERTIES
			IMPORTED_IMPLIB "C:/glew-2.1.0/lib/Release/x64/glew32.lib"
			LINKER_LANGUAGE C
		)
		target_link_libraries(destoer-emu GLEW)

	endif()


	# sdl (allways needed)
    add_library(SDL2 SHARED IMPORTED)
    set_target_properties(SDL2 PROPERTIES
        IMPORTED_LOCATION "C:/SDL2-2.0.14/lib/x64/SDL2.dll"
        IMPORTED_IMPLIB "C:/SDL2-2.0.14/lib/x64/SDL2.lib"
        LINKER_LANGUAGE C
    )
	target_link_libraries(destoer-emu SDL2)
	
    include_directories("C:/SDL2-2.0.14/include")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Werror -Wall -Wextra -march=native -pthread -Ofast -fomit-frame-pointer -Wno-missing-braces")
if(${FRONTEND} STREQUAL "HEADLESS")

else ()
	target_link_libraries(destoer-emu SDL2)
endif()

if(${FRONTEND} STREQUAL "IMGUI") 
	target_link_libraries(destoer-emu GL GLEW)
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