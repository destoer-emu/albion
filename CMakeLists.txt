#project config

cmake_minimum_required(VERSION 3.10)

project(albion)
include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(FRONTEND "SDL")
#set(FRONTEND "IMGUI")
#set(FRONTEND "DESTOER")

# external frontend
#set(FRONTEND "HEADLESS")

include("thirdparty/spdlog.cmake")
include("thirdparty/sdl2.cmake")

if(${FRONTEND} STREQUAL "IMGUI")
	include("thirdparty/glew.cmake")
	include("thirdparty/imgui.cmake")
endif()

#set(GB "TRUE")
set(GBA "TRUE")
#set(N64 "TRUE")

# add_compile_options(-fsanitize=undefined)
# add_link_options(-fsanitize=undefined)

#add_definitions(-DBOUNDS_CHECK)
if(${FRONTEND} STREQUAL "IMGUI")
	add_definitions(-DAUDIO_ENABLE -DSDL_REQUIRED -DAUDIO_SDL -DCONTROLLER_SDL -DFETCH_SPEEDHACK)
	add_definitions(-DFRONTEND_IMGUI -DIMGUI_IMPL_OPENGL_LOADER_GLEW -DDEBUG -DLOG_CONSOLE)
endif()

if(${FRONTEND} STREQUAL "SDL")
	add_definitions(-DAUDIO_ENABLE -DSDL_REQUIRED -DAUDIO_SDL -DDEBUG -DCONTROLLER_SDL -DFETCH_SPEEDHACK)
	add_definitions(-DFRONTEND_SDL)
endif()


if(${FRONTEND} STREQUAL "DESTOER")
	add_definitions(-DAUDIO_ENABLE -DSDL_REQUIRED -DAUDIO_SDL -DCONTROLLER_SDL -DDEBUG -DFETCH_SPEEDHACK)
	add_definitions(-DFRONTEND_DESTOER)
endif()

if(${FRONTEND} STREQUAL "HEADLESS")
	add_definitions(-DFRONTEND_HEADLESS)
endif()


# source files 

# main source files
file(GLOB src_files
	"src/main.cpp"
	"src/common/albion/*.cpp"
	"src/frontend/gb/*.cpp"
	"src/frontend/gba/*.cpp"
	"src/frontend/*.cpp"
)

# add some includes
include_directories("src/common/albion/headers")
include_directories("src/common/thirdparty")
include_directories("src/")

if(${FRONTEND} STREQUAL "SDL")
	file(GLOB frontend_files
		"src/frontend/sdl/sdl_window.cpp"
	)
endif()

if(${FRONTEND} STREQUAL "DESTOER")
	file(GLOB frontend_files
		"src/frontend/destoer/*.cpp"
	)

	include_directories("src/frontend/destoer")
endif()

if(${FRONTEND} STREQUAL "IMGUI")
	file(GLOB frontend_files
		"src/frontend/imgui/*.cpp"
	)
endif()

add_executable(albion  ${src_files} ${frontend_files})

if(WIN32)

else()
	# prevent O3 being injected
	SET( CMAKE_CXX_FLAGS_DEBUG  "")
	SET( CMAKE_CXX_FLAGS_RELEASE  "")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Werror -Wall -Wextra -march=native -pthread -O0 -fomit-frame-pointer -Wno-missing-braces")
endif()

add_subdirectory("destoer-lib")

if(DEFINED N64)
	add_subdirectory("src/n64")
	add_subdirectory("beyond-all-repair")
	target_link_libraries(albion PUBLIC n64)
endif()

# build gb psg in seperate lib as two cores depend on it
if(DEFINED GB OR DEFINED GBA)
	add_subdirectory("src/psg")
endif()

if(DEFINED GBA)
	add_subdirectory("src/gba")
	target_link_libraries(albion PUBLIC gba)
endif()

if(DEFINED GB)
	add_subdirectory("src/gb")
	target_link_libraries(albion PUBLIC gb)
endif()

if(${FRONTEND} STREQUAL "IMGUI") 
	target_link_libraries(albion PUBLIC imgui)
endif()

target_link_libraries(albion PRIVATE destoer spdlog::spdlog SDL2::SDL2-static)