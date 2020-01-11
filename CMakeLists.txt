#project config

cmake_minimum_required(VERSION 3.10)

project(destoer-emu)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


#set(FRONTEND "QT")
set(FRONTEND "SDL")
#set(FRONTEND "IMGUI")

if(${FRONTEND} STREQUAL "IMGUI")
	add_definitions(-DFRONTEND_IMGUI -DIMGUI_IMPL_OPENGL_LOADER_GLEW -DDEBUG -DLOG_CONSOLE)
endif()

if(${FRONTEND} STREQUAL "SDL")
	add_definitions(-DFRONTEND_SDL)
endif()

if(${FRONTEND} STREQUAL "QT") # not supported yet come back to this
	add_definitions(-DFRONTEND_QT)
endif()


# make these paths less of a pain to modify :)

if(WIN32)

	list(APPEND CMAKE_PREFIX_PATH "C:/SDL2-2.0.10")

	if(${FRONTEND} STREQUAL "IMGUI")  # needed only for imgui 
		# external libs 
		list(APPEND CMAKE_PREFIX_PATH "C:/glew-2.1.0")

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
	endif()


	# sdl allways needed
	find_package(SDL2 REQUIRED)
	if(SDL2_FOUND)
		include_directories(${SDL2_INCLUDE_DIRS})
		#link_libraries(${SDL2_LIBRARIES})
		link_libraries("C:/SDL2-2.0.10/lib/x64/SDL2.lib")
	endif()
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Werror -Wall -Wextra -pthread -Wshadow -O3")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lSDL2")
	if(${FRONTEND} STREQUAL "IMGUI") 
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lglfw  -lGL -lGLEW")
	endif()
endif()



if(${FRONTEND} STREQUAL "QT") 
	set(CMAKE_AUTOMOC ON)
	set(CMAKE_AUTORCC ON)
	set(CMAKE_AUTOUIC ON)
	find_package(Qt5 COMPONENTS Widgets REQUIRED)
	link_libraries(Qt5::Widgets)
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
)

if(${FRONTEND} STREQUAL "IMGUI")
	file(GLOB frontend_files
		"src/frontend/imgui/*.h"
		"src/frontend/imgui/*.cpp"
		"src/lib/imgui/*.cpp"
		"src/lib/imgui/*.h"	
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

add_executable(destoer-emu ${src_files} ${frontend_files})