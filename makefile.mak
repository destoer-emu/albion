CFLAGS = -O3 -DDEBUG -std=c++17 -Wall -Werror -Wextra -DFRONTEND_IMGUI -DIMGUI_IMPL_OPENGL_LOADER_GLEW
TARGET = emu
LDFLAGS = 
# if on windows need extra flags
# if on windows need extra flags
ifeq ($(OS),Windows_NT)
	LDFLAGS += -lmingw32 -lSDL2main -lSDL2
else 
	LDFLAGS = -lSDL2 -lglfw -lGL -lGLEW -lpthread
endif
CC = g++
OBJDIR=obj
DIRS = src src/fmt src/sdl src/imgui

CFILES = $(foreach DIR,$(DIRS),$(wildcard $(DIR)/*.cpp))
COBJFILES = $(CFILES:%.cpp=$(OBJDIR)/%.o)

$(TARGET): $(COBJFILES)
	$(CC) $(CFLAGS) $(COBJFILES) -o $(TARGET) $(LDFLAGS)

$(COBJFILES): $(OBJDIR)/%.o : %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

