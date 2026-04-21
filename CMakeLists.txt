cmake_minimum_required(VERSION 4.0)
project(RealtimeMemoryForensics LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_COLOR_DIAGNOSTICS ON)
add_compile_options(
    -fmacro-prefix-map=${CMAKE_SOURCE_DIR}=. 
    -fdebug-prefix-map=${CMAKE_SOURCE_DIR}=.
    -ffile-prefix-map=${CMAKE_SOURCE_DIR}=.
    -fdiagnostics-color=always
)
set(CXX_COMP_COMMANDS
    -Wall -Wextra -Wpedantic -Wno-cpp
    -fdiagnostics-show-template-tree
    -g
    $<$<CONFIG:Debug>:-O0> $<$<CONFIG:Release>:-O3>
)

include(FetchContent)
# ---------------- magic_enum ----------------
FetchContent_Declare(
    magic_enum
    GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
    GIT_TAG v0.9.7
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(magic_enum)

file(GLOB_RECURSE librmf_src_files "./src/*.cpp")
message("-- librmf src files: ${librmf_src_files}")
add_library(librmf STATIC ${librmf_src_files})
target_link_libraries(librmf PRIVATE
    magic_enum::magic_enum
)
target_compile_options(librmf PUBLIC ${CXX_COMP_COMMANDS})
target_include_directories(librmf PUBLIC "include")

option(tests "Compile tests" ON)
if(tests)
    enable_testing()
    add_subdirectory(tests)
endif()
