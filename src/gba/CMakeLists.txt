set(gba_files
    src/apu/apu.cpp

    src/cpu/arm_disass.cpp
    src/cpu/arm_opcode.cpp
    src/cpu/arm.cpp
    src/cpu/cpu.cpp
    src/cpu/swi.cpp
    src/cpu/thumb_disass.cpp
    src/cpu/thumb_opcode.cpp

    src/memory/apu_io.cpp
    src/memory/cpu_io.cpp
    src/memory/disp_io.cpp
    src/memory/dma.cpp
    src/memory/flash.cpp
    src/memory/mem_io.cpp
    src/memory/memory.cpp 
    src/memory/waitstate.cpp

    src/ppu/display_gfx.cpp
    src/ppu/display.cpp
    src/ppu/sprite.cpp
    src/ppu/viewer.cpp

    src/debug.cpp
    src/gba.cpp
    src/scheduler.cpp
)

add_library(gba STATIC ${gba_files})
target_compile_definitions(gba PUBLIC -DGBA_ENABLED)
target_include_directories(gba PUBLIC "headers/")

target_link_libraries(gba PUBLIC spdlog::spdlog psg)