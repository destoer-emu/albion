#project config

cmake_minimum_required(VERSION 3.21)

project(albion)
include(FetchContent)

macro(thirdparty_module name)
	FetchContent_Declare(${name}
			${ARGN}
			PREFIX "${CMAKE_CURRENT_BINARY_DIR}/${name}"
			TMP_DIR "${CMAKE_CURRENT_BINARY_DIR}/${name}/tmp"
			STAMP_DIR "${CMAKE_CURRENT_BINARY_DIR}/${name}/stamp"
			DOWNLOAD_DIR "${CMAKE_CURRENT_BINARY_DIR}/${name}/dl"
			SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/${name}/src"
			SUBBUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/${name}/build"
			BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/${name}/bin"
			INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/${name}/inst"
			LOG_DIR "${CMAKE_CURRENT_BINARY_DIR}/${name}/log"
			)
	list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/cmake/${name}")
endmacro()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(FRONTEND "SDL")
#set(FRONTEND "IMGUI")
#set(FRONTEND "DESTOER")

# external frontend
#set(FRONTEND "HEADLESS")

thirdparty_module(spdlog
		URL https://github.com/gabime/spdlog/archive/refs/tags/v1.14.1.zip
)

set(SPDLOG_INSTALL ON)
set(SPDLOG_ENABLE_PCH ON)

FetchContent_MakeAvailable(spdlog)
set_property(TARGET spdlog PROPERTY POSITION_INDEPENDENT_CODE ON)


#add_definitions(-DBOUNDS_CHECK)
if(${FRONTEND} STREQUAL "IMGUI")
	add_definitions(-DAUDIO_ENABLE -DSDL_REQUIRED -DAUDIO_SDL -DCONTROLLER_SDL -DFETCH_SPEEDHACK)
	add_definitions(-DFRONTEND_IMGUI -DIMGUI_IMPL_OPENGL_LOADER_GLEW -DDEBUG -DLOG_CONSOLE)

	#set(GB "TRUE")
	#set(GBA "TRUE")
	set(N64 "TRUE")

endif()

if(${FRONTEND} STREQUAL "SDL")
	add_definitions(-DAUDIO_ENABLE -DSDL_REQUIRED -DAUDIO_SDL -DDEBUG -DCONTROLLER_SDL -DFETCH_SPEEDHACK)
	add_definitions(-DFRONTEND_SDL)

	set(N64 "TRUE")
	#set(GB "TRUE")
	#set(GBA "TRUE")
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
include_directories("src/headers")
include_directories("src/common/thirdparty")

include_directories("src")

#destoer lib
include_directories("destoer-lib/src/")
add_definitions(-DFORWARD_DEF_DESTOER_LIB)

if(${FRONTEND} STREQUAL "IMGUI")
	include_directories("/usr/include/SDL2")
	include_directories("src/common/thirdparty/imgui")
	file(GLOB frontend_files
		"src/frontend/imgui/*.cpp"
		"src/common/thirdparty/imgui/*.cpp"
		"src/common/thirdparty/imgui/backends/imgui_impl_sdl2.cpp"
		"src/common/thirdparty/imgui/backends/imgui_impl_opengl3.cpp"	
	)
endif()


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



add_executable(albion  ${src_files} ${frontend_files})

# make these paths less of a pain to modify :)

if(WIN32)
	if(${FRONTEND} STREQUAL "IMGUI")  # needed only for imgui 
		# external libs

		# opengl
		find_package(OPENGL REQUIRED)
		target_link_libraries(albion PUBLIC OpenGL::GL)

		# glew
		list(APPEND CMAKE_PREFIX_PATH "C:/glew-2.1.0")
		find_package(GLEW REQUIRED)
		target_link_libraries(albion GLEW::GLEW)

		# for some reason building is broken and extra steps are required
		add_library(GLEW SHARED IMPORTED)
		set_target_properties(GLEW PROPERTIES
			IMPORTED_IMPLIB "C:/glew-2.1.0/lib/Release/x64/glew32.lib"
			LINKER_LANGUAGE C
		)
		target_link_libraries(albion PUBLIC GLEW)

	endif()

else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Werror -Wall -Wextra -march=native -pthread -O1 -fomit-frame-pointer -Wno-missing-braces")

	if(${FRONTEND} STREQUAL "HEADLESS")

	else ()
		target_link_libraries(albion PUBLIC SDL2)
	endif()
endif()

if(DEFINED N64)
	add_definitions(-DN64_ENABLED)
	add_library(n64 STATIC "src/n64/n64.cpp")
	include_directories("beyond-all-repair/src")
	target_link_libraries(n64 spdlog)

	add_custom_target(mips_lut COMMAND ${CMAKE_SOURCE_DIR}/tools/lutgen.exe ${CMAKE_SOURCE_DIR}/src/n64/instr/mips_lut.cpp)

	set_source_files_properties(${CMAKE_SOURCE_DIR}/src/n64/instr/mips_lut.cpp PROPERTIES GENERATED TRUE)

	add_dependencies(n64 mips_lut)

	target_link_libraries(albion PUBLIC n64)
endif()

if(DEFINED GBA)
	file(GLOB gba_files
		"src/gba/*.cpp"
		"src/gba/cpu/*.cpp"
		"src/gba/memory/*.cpp"
		"src/gba/ppu/*.cpp"
		"src/gba/apu/*.cpp"
	)

	add_definitions(-DGBA_ENABLED)
	add_library(gba STATIC ${gba_files})

	# TODO: gba requires gb for psg support 
	# we need to split the files up

	target_link_libraries(albion PUBLIC gba)
endif()

if(DEFINED GB)
	file(GLOB gb_files
		"src/gb/*.cpp"	
		"src/gb/cpu/*.cpp"
		"src/gb/apu/*.cpp"
		"src/gb/ppu/*.cpp"
		"src/gb/memory/*.cpp"
	)
	add_definitions(-DGB_ENABLED)

	add_library(gb STATIC ${gb_files})

	target_link_libraries(albion PUBLIC gb)
endif()

# build gb psg in seperate lib as two cores depend on it
if(DEFINED GB OR DEFINED GBA)
	file(GLOB psg_files
		"src/gb/apu/psg/*cpp"
	)

	add_library(psg STATIC ${psg_files})

	target_link_libraries(albion PUBLIC psg)
endif()


if(${FRONTEND} STREQUAL "IMGUI") 
	target_link_libraries(albion PUBLIC GL GLEW)
endif()

target_link_libraries(albion PUBLIC spdlog)

add_subdirectory(beyond-all-repair)

if(WIN32)
	# Fetch SDL for the runtime
	FetchContent_Declare(
			SDL2
			URL https://www.libsdl.org/release/SDL2-devel-2.30.2-VC.zip
	)
	FetchContent_MakeAvailable(SDL2)
	set(SDL2_DIR ${sdl2_SOURCE_DIR})
	list(APPEND CMAKE_PREFIX_PATH "${sdl2_SOURCE_DIR}/cmake")

	find_package(SDL2 REQUIRED)

	target_link_libraries(albion PUBLIC SDL2::SDL2)

	add_custom_command(
			TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
			$<TARGET_FILE:SDL2::SDL2>
			$<TARGET_FILE_DIR:${PROJECT_NAME}>
			VERBATIM)
endif()
