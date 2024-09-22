

set(psg_files
    src/channel.cpp
    src/envelope.cpp
    src/freq.cpp
    src/noise.cpp
    src/psg.cpp
    src/square.cpp
    src/sweep.cpp
    src/wave.cpp
)

add_library(psg STATIC ${psg_files})
target_include_directories(psg PUBLIC "headers/")

target_link_libraries(psg PUBLIC spdlog::spdlog destoer)