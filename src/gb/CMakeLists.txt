

set(gb_files
    src/debug.cpp
    src/gb.cpp
    src/scheduler.cpp

    src/apu/apu.cpp
    src/apu/apu_save_state.cpp

    src/cpu/cpu_save_state.cpp
    src/cpu/cpu.cpp
    src/cpu/disass.cpp
    src/cpu/opcode.cpp

    src/memory/banking.cpp
    src/memory/memory_save_state.cpp
    src/memory/memory.cpp
    src/memory/rom.cpp

    src/ppu/ppu_save_state.cpp
    src/ppu/ppu.cpp
    src/ppu/render.cpp
    src/ppu/viewer.cpp    
)

add_library(gb STATIC ${gb_files})
target_compile_definitions(gb PUBLIC -DGB_ENABLED)
target_include_directories(gb PUBLIC "headers/")

target_link_libraries(gb PUBLIC spdlog::spdlog psg destoer)