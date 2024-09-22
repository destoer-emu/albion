include(FetchContent)
FetchContent_Declare(
        GLEW
        GIT_REPOSITORY https://github.com/Perlmint/glew-cmake
        GIT_TAG glew-cmake-2.2.0
)
FetchContent_MakeAvailable(GLEW)