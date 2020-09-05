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


# todo

fix regressions from gb scheduler, 
multithread scanline renderer,
fix save states,

gba bitmap affine transforms,
gba bitmap alpha blending,
256 color sprite fix,
mosaic,
open bus,
cpu pipeline,
bus limitations,
memory timing (seq, nonseq),
gamepak prefetch,
waitcnt,
gb psg port to gba,
save support,
gba vram viewer

# thanks

# gba
fleroviux https://github.com/fleroviux/

YetAnotherEmuDev https://github.com/YetAnotherEmuDev

rockpolish https://github.com/RockPolish

dillon https://github.com/Dillonb

ladystarbreeze https://github.com/ladystarbreeze

# gb
gekkio https://github.com/gekkio

mattcurrie https://github.com/mattcurrie

LIJI https://github.com/LIJI32

and anyone else i missed

# resources used
https://problemkaputt.de/gbatek.htm

https://gbdev.gg8.se/wiki/articles/Pan_Docs

https://www.coranac.com/tonc/text/