multi system emulator written using c++17 with support for gameboy and wip support for gba


![alt text](https://raw.githubusercontent.com/destoer/destoer-emu/master/pics/qt.png)
![alt text](https://raw.githubusercontent.com/destoer/destoer-emu/master/pics/sdl.png)
![alt text](https://raw.githubusercontent.com/destoer/destoer-emu/master/pics/imgui.png)

# Frontends:

Frontends using SDL, QT and IMGUI are implemented
with sdl being the most basic and qt offering basic file dialogs
and imgui additonally having a debugger

the frontend to build can be configured at the top of the cmake file.

Imgui depends on glfw, opengl & glew
all builds (even qt) depend on sdl currently for sound.

# status: 
gameboy is mostly finished with most features supported and only accuracy fixes needed
mem-timing, instr-timing, halt_bug and cpu_instrs are passing from blarggs tests
and the current test passes of gekkios test suite can be seen in TEST_RESULT

gba support is very early and can only boot test roms such as armwrestler

