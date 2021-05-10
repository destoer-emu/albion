multi system emulator written using c++17 with support for gameboy and wip support for gba


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

gba support is very early and can run a few games but is not well optimised
and not very accurate


# todo

not really necessary but would be nice for gb
serial, sgb, dmg cgb color palettes


gba bitmap affine transforms,
gba bitmap alpha blending,
mosaic,
open bus,
cpu pipeline,
bus limitations,
instr timing rewrite
memory timing (seq, nonseq),
gamepak prefetch,
waitcnt,
gb psg port to gba,
gba vram viewer

# thanks

# gba
fleroviux https://github.com/fleroviux/

YetAnotherEmuDev https://github.com/YetAnotherEmuDev

rockpolish https://github.com/RockPolish

dillon https://github.com/Dillonb

ladystarbreeze https://github.com/ladystarbreeze

DenSinH https://github.com/DenSinH/GBARoms

# gb
gekkio https://github.com/gekkio

mattcurrie https://github.com/mattcurrie

LIJI https://github.com/LIJI32

# n64
dillon https://github.com/Dillonb

and anyone else i missed

# resources used
https://problemkaputt.de/gbatek.htm

https://gbdev.gg8.se/wiki/articles/Pan_Docs

https://www.coranac.com/tonc/text/

https://n64.readthedocs.io/index.html